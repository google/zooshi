#!/usr/bin/python
# Copyright 2014 Google Inc. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Builds all assets under src/rawassets/, writing the results to assets/.

Finds the flatbuffer compiler and cwebp tool and then uses them to convert the
JSON files to flatbuffer binary files and the png files to webp files so that
they can be loaded by the game. This script also includes various 'make' style
rules. If you just want to build the flatbuffer binaries you can pass
'flatbuffer' as an argument, or if you want to just build the webp files you can
pass 'cwebp' as an argument. Additionally, if you would like to clean all
generated files, you can call this script with the argument 'clean'.
"""

import argparse
import distutils.dep_util
import distutils.spawn
import glob
import json
import logging
import math
import os
import platform
import shutil
import subprocess
import sys


class DependencyPathError(Exception):
  """Thrown if a dependency isn't found.

  Attributes:
    filename: File not found.
    paths: Paths searched for the file.
  """

  def __init__(self, error_message, filename, paths):
    self.filename = filename
    self.paths = paths
    super(DependencyPathError, self).__init__(error_message)


class DependencyPath(object):
  """Path to a project dependency (e.g fplbase, pindrop, motive etc).

  Attributes:
    project_relative_path: Path to the dependency directory relative to the
      project's directory.
    final_absolute_path: Absolute path to the dependency, populated when the
      DependencyPath.resolve() method is called.

  Class Attributes:
    path_resolvers: List of path resolver Callables which are called in order
      by resolve() to translate each project_relative_path to the
      final_absolute_path.  The callable takes a single string argument and
      returns a string if the path is resolved or an empty string if the
      resolver did not process the string.  Once a path is resolved no
      additional resolvers are called on the path.
  """

  path_resolvers = []

  def __init__(self, project_relative_path):
    """Initialize this instance.

    Args:
      project_relative_path: Path of the dependency relative to the project
        directory.
    """
    self.project_relative_path = project_relative_path
    self.final_absolute_path = ''

  def join(self, *relative_path):
    """Join components to this path returning a new instance..

    Args:
      *relative_path: Arguments that - when joined - specify the Relative path
       from this path.

    Returns:
      DependencyPath instance for the new path.
    """
    join_list = list(relative_path)
    join_list.insert(0, self.project_relative_path)
    return DependencyPath(os.path.join(*join_list))

  def resolve(self, log_errors=True):
    """Resolve the project_relative_path to final_absolute_path.

    See DependencyPath.path_resolvers for details on how the path is
    resolved.

    Args:
      log_errors: Log errors if it's not possible to resolve the path.

    Returns:
      Absolute path to the resource.

    Raises:
      DependencyPathError: If a project isn't found.
    """
    # If no resolvers are present it's not possible to convert paths.
    assert DependencyPath.path_resolvers
    if self.final_absolute_path:
      return self.final_absolute_path
    candidate_paths = []
    path_found = ''
    for resolver in DependencyPath.path_resolvers:
      candidate_path = resolver(self.project_relative_path)
      if candidate_path:
        candidate_paths.append((candidate_path, resolver))
        if not path_found and os.path.exists(candidate_path):
          path_found = candidate_path

    if not path_found:
      if log_errors:
        for candidate_path, resolver in candidate_paths:
          if not os.path.exists(candidate_path):
            logging.error('%s not found at %s (resolver %s)',
                          os.path.basename(self.project_relative_path),
                          candidate_path, repr(resolver))
      raise DependencyPathError('%s not found' % self.project_relative_path,
                                self.project_relative_path, candidate_paths)
    self.final_absolute_path = path_found
    return self.final_absolute_path

  def __str__(self):
    """Returns the relative path."""
    return self.project_relative_path

  def __repr__(self):
    """Returns the relative path."""
    return self.__str__()

  @classmethod
  def add_resolver(cls, resolver):
    """Add a resolver lambda to the list of path resolvers.

    Args:
      resolver: Callable object used to resolve a path.
        See DependencyPath.path_resolvers for details on how the path is
        resolved.
    """
    cls.path_resolvers.insert(0, resolver)


class DefaultPathResolver(object):
  """Converts project relative paths to absolute paths.

  Attributes:
    project_root: Absolute path to the project used to generate resolve all
      absolute paths.
    prebuilts_root: Directory containing prebuilt (binary) libraries / tools.
    fpl_root: Directory containing Google libraries.
    third_party_root: Directory containing non-Google libraries.
    tag_path_map: Map of tags (path prefixes) to relative paths.

  Class Attributes:
    DEPENDENCIES_DIR: Location of dependencies in a flat directory structure.
    SOURCE_TREE_ROOT: Root of the internal (non-distributed) source tree.
    DEFAULT_FPL_ROOT: Path relative to the project of the FPL library directory.
    PREBUILTS_ROOT: Tag used to indicate the path is relative to prebuilts_root.
    FPL_ROOT: Tag used to indicate the path is relative to fpl_root.
    THIRD_PARTY_ROOT: Tag used to indicate the path is relative to
       third_party_root.
  """

  DEPENDENCIES_DIR = 'dependencies'
  SOURCE_TREE_ROOT = os.path.join(os.path.pardir, os.path.pardir,
                                  os.path.pardir, os.path.pardir)
  DEFAULT_FPL_ROOT = os.path.join(os.path.pardir, os.path.pardir, 'libs')
  FPL_ROOT = '__libs__'
  PREBUILTS_ROOT = '__prebuilts__'
  THIRD_PARTY_ROOT = '__external__'

  def __init__(self, project_root):
    """Initialize the instance.

    Initializes prebuilts_root, third_party_root and fpl_root based upon the
    existance of DEPENDENCIES_DIR relative to project_root.

    Args:
      project_root: Absolute path to the project.
    """
    self.project_root = project_root
    if os.path.exists(os.path.join(self.project_root,
                                   DefaultPathResolver.DEPENDENCIES_DIR)):
      self.prebuilts_root = DefaultPathResolver.DEPENDENCIES_DIR
      self.third_party_root = DefaultPathResolver.DEPENDENCIES_DIR
      self.fpl_root = DefaultPathResolver.DEPENDENCIES_DIR
    else:
      self.prebuilts_root = os.path.join(DefaultPathResolver.SOURCE_TREE_ROOT,
                                         'prebuilts')
      self.third_party_root = os.path.join(DefaultPathResolver.SOURCE_TREE_ROOT,
                                           'external')
      self.fpl_root = DefaultPathResolver.DEFAULT_FPL_ROOT
    self.tag_path_map = {
        DefaultPathResolver.FPL_ROOT: self.fpl_root,
        DefaultPathResolver.PREBUILTS_ROOT: self.prebuilts_root,
        DefaultPathResolver.THIRD_PARTY_ROOT: self.third_party_root}

  def __call__(self, path):
    """Map the specified path to a resource path.

    Args:
      path: Project relative path to map to a resource.

    Returns:
      Mapped path to the resource if the path starts with one of PREBUILTS_ROOT,
      FPL_ROOT or THIRD_PARTY_ROOT.  If the path is not matched it's simply
      expanded into an absolute path from the project directory.
    """
    for tag, mapped_path in self.tag_path_map.iteritems():
      if path.startswith(tag):
        return os.path.abspath(os.path.join(
            self.project_root, str(path).replace(tag, mapped_path)))
    return os.path.abspath(os.path.join(self.project_root, path))


class ResourcePath(object):
  """Path to a resource.

  Attributes:
    dependency_paths: List of DependencyPath instances that reference
      optional paths for resource_file.
    resource_file: File located in one of dependency_paths.  This is populated
      by resolve().
  """

  def __init__(self, dependency_paths, resource_file):
    """Initialize the instance.

    Args:
      dependency_paths: List of DependencyPath instances that reference
        optional paths for resource_file.  If an entry in the list is a string
        it is wrapped in a DependencyPath instance.
      resource_file: File located in one of dependency_paths.
    """
    assert dependency_paths
    self.dependency_paths = []
    for path in dependency_paths:
      if issubclass(path.__class__, DependencyPath):
        self.dependency_paths.append(path)
      else:
        self.dependency_paths.append(DependencyPath(path))
    self.resource_file = resource_file

  def resolve(self, raise_on_error=True):
    """Resolve the path to a resource.

    Searches dependency_paths (resolving the path for each one) looking for
    the presence of resource_file.

    Args:
      raise_on_error: Whether to raise an exception if the file isn't found.

    Returns:
      Path to the resource if it's found.  Returns an empty string if the
      file isn't found and raise_on_error is False.

    Raises:
      DependencyPathError: If the resource isn't found.
    """
    for path in self.dependency_paths:
      try:
        resolved_path = path.resolve(log_errors=False)
      except DependencyPathError:
        continue
      resolved_file = os.path.join(resolved_path, self.resource_file)
      if os.path.exists(resolved_file):
        return resolved_file
    if raise_on_error:
      raise DependencyPathError(
          '%s not found in %s' % (self.resource_file,
                                  str(self.dependency_paths)),
          self.resource_file, self.dependency_paths)
    return ''


class BinaryPath(ResourcePath):
  """Path to an executable binary.

  Class Attributes:
    EXECUTABLE_EXTENSION: Executable file extension.
  """

  EXECUTABLE_EXTENSION = 'exe' if platform.system() == 'Windows' else ''

  def __init__(self, dependency_paths, binary_file):
    """Initialize the instance.

    Args:
      dependency_paths: List of DependencyPath instances that reference
        optional paths for resource_file.
      binary_file: File located in one of dependency_paths or the system path.
    """
    dependency_paths = list(dependency_paths)
    binary_file_path = distutils.spawn.find_executable(binary_file)
    if binary_file_path:
      dependency_paths.append(DependencyPath(
          os.path.dirname(binary_file_path)))
      self.resource_file = os.path.basename(binary_file_path)
    else:
      self.resource_file = os.path.extsep.join(
          (os.path.splitext(binary_file)[0], BinaryPath.EXECUTABLE_EXTENSION))
    super(BinaryPath, self).__init__(dependency_paths, binary_file)


class ImageMagickPathResolver(DefaultPathResolver):
  """Finds the binary directory for ImageMagick.

  Class Attributes:
    IMAGEMAGICK_BIN: Directory containing ImageMagick binaries.
    CONVERT_EXECUTABLE: Name of the convert executable.
  """

  IMAGEMAGICK_BIN = '__imagemagick__'
  CONVERT_EXECUTABLE = 'convert' + BinaryPath.EXECUTABLE_EXTENSION

  def __init__(self, project_root):
    """Initialize the instance."""
    super(ImageMagickPathResolver, self).__init__(project_root)

  def __call__(self, path):
    """Resolve the specified path.

    Args:
      path: Path to resolve.

    Returns:
      Path with IMAGEMAGICK_BIN and everything defined by DefaultPathResolver
      expanded.
    """
    default_resolution = super(ImageMagickPathResolver, self).__call__(path)
    if default_resolution:
      path = default_resolution
    components = path.split(os.path.sep)
    if ImageMagickPathResolver.IMAGEMAGICK_BIN not in components:
      return path

    component_index = components.index(ImageMagickPathResolver.IMAGEMAGICK_BIN)
    path_up_to_dir = os.path.sep.join(components[:component_index])
    imagemagick_binary_dir = ''
    for root, _, filenames in os.walk(os.path.join(
        path_up_to_dir, 'imagemagick',
        '%s-x86_64' % platform.system().lower()), followlinks=True):
      if ImageMagickPathResolver.CONVERT_EXECUTABLE in filenames:
        imagemagick_binary_dir = root
        break
    if not imagemagick_binary_dir:
      return ''
    return os.path.sep.join([imagemagick_binary_dir] +
                            components[component_index + 1:])


# TODO: Use distutils.dep_util.
class FlatbuffersConversionData(object):
  """Holds data needed to convert a set of json files to flatbuffer binaries.

  Attributes:
    schema: The path to the flatbuffer schema file.
    input_files: A list of input files to convert.
  """

  def __init__(self, schema, extension, input_files):
    """Initializes this object's schema and input_files.

    Args:
      schema: ResourcePath instance to the schema to use when processing the
        files.
      extension: Extension to add to the schema.
      input_files: Files to convert to binary data.
    """
    self.schema = schema
    self.extension = extension
    self.input_files = input_files


class BuildError(Exception):
  """Error indicating there was a problem building assets."""

  def __init__(self, argv, error_code, message=None):
    Exception.__init__(self)
    self.argv = argv
    self.error_code = error_code
    self.message = message if message else ''


def run_subprocess(argv, capture=False):
  """Run a command line application.

  Args:
    argv: The command line application path and arguments to pass to it.
    capture: Whether to capture the standard output stream of the application
      and return it from this method.

  Returns:
    Standard output of the command line application if capture=True, None
    otherwise.

  Raises:
    BuildError: If the command fails.
  """
  if VERBOSE:
    logging.debug(*argv)
  try:
    if capture:
      process = subprocess.Popen(args=argv, bufsize=-1, stdout=subprocess.PIPE)
    else:
      process = subprocess.Popen(argv)
  except OSError as e:
    raise BuildError(argv, 1, message=str(e))
  stdout, _ = process.communicate()
  if process.returncode:
    raise BuildError(argv, process.returncode)
  return stdout


# Directories containing projects this script depends upon.
BREADBOARD_ROOT = DependencyPath(os.path.join(DefaultPathResolver.FPL_ROOT,
                                              'breadboard'))
CORGI_ROOT = DependencyPath(os.path.join(DefaultPathResolver.FPL_ROOT,
                                         'entity'))
FLATBUFFERS_ROOT = DependencyPath(os.path.join(DefaultPathResolver.FPL_ROOT,
                                               'flatbuffers'))
FPLBASE_ROOT = DependencyPath(os.path.join(DefaultPathResolver.FPL_ROOT,
                                           'fplbase'))
MOTIVE_ROOT = DependencyPath(os.path.join(DefaultPathResolver.FPL_ROOT,
                                          'motive'))
PINDROP_ROOT = DependencyPath(os.path.join(DefaultPathResolver.FPL_ROOT,
                                           'pindrop'))
SCENE_LAB_ROOT = DependencyPath(os.path.join(DefaultPathResolver.FPL_ROOT,
                                             'scene_lab'))

# Location of FlatBuffers compiler.
# This is usually built from source in which case the binary directory is
# specified as a script argument.
FLATC = BinaryPath(
    ['bin',
     os.path.join('bin', 'Release'),
     os.path.join('bin', 'Debug'),
     str(FLATBUFFERS_ROOT),
     os.path.join(str(FLATBUFFERS_ROOT), 'bin'),
     os.path.join(str(FLATBUFFERS_ROOT), 'Debug'),
     os.path.join(str(FLATBUFFERS_ROOT), 'Release')], 'flatc')

# Location of webp compression tool.
CWEBP = BinaryPath(
    ['bin',
     os.path.join('bin', 'Release'),
     os.path.join('bin', 'Debug'),
     os.path.join(DefaultPathResolver.PREBUILTS_ROOT,
                  'libwebp', '%s-x86' % platform.system().lower(),
                  'libwebp-0.4.1-%s-x86-32' % platform.system().lower(),
                  'bin')], 'cwebp')

# Location of mesh_pipeline conversion tool.
MESH_PIPELINE = BinaryPath(
    [os.path.join(str(FPLBASE_ROOT), 'bin', platform.system(), p)
     for p in ('', 'Debug', 'Release')], 'mesh_pipeline')

# Location of the anim_pipeline conversion tool.
ANIM_PIPELINE = BinaryPath(
    [os.path.join(str(MOTIVE_ROOT), 'bin', platform.system(), p)
     for p in ('', 'Debug', 'Release')], 'anim_pipeline')

# Location of imagemagick tools.
IMAGEMAGICK_BINARY_DIR = os.path.join(
    DefaultPathResolver.PREBUILTS_ROOT, ImageMagickPathResolver.IMAGEMAGICK_BIN)
IMAGEMAGICK_IDENTIFY = BinaryPath([IMAGEMAGICK_BINARY_DIR], 'identify')
IMAGEMAGICK_CONVERT = BinaryPath([IMAGEMAGICK_BINARY_DIR], 'convert')

# Graphics magick optionally overrides imagemagick tools.
GRAPHICSMAGICK = BinaryPath([''], 'gm')

# What level of quality we want to apply to the webp files.
# Ranges from 0 to 100.
WEBP_QUALITY = 90

# Maximum width or height of a tga image
MAX_TGA_SIZE = 1024

# Display commands executed by run_subprocess().
VERBOSE = False


# =============================================================================
# Project specific constants below
# =============================================================================

# The project root directory, which is one level up from this script's
# directory.
PROJECT_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__),
                                            os.path.pardir))

# ============================================================================
# Below are the *internal* resource paths (i.e these are static and don't
# move when dependent projects are relocated).
# ============================================================================

# Directory to place processed assets.
ASSETS_PATH = os.path.join(PROJECT_ROOT, 'assets')

# Directory where unprocessed assets can be found.
RAW_ASSETS_PATH = os.path.join(PROJECT_ROOT, 'src', 'rawassets')

# Metadata file for assets. Store extra file-specific information here.
ASSET_META = os.path.join(RAW_ASSETS_PATH, 'asset_meta.json')

# Directory for unprocessed assets not in the main code-base.
# NOTE: No need for expansion as this isn't redistributed.
INTERNAL_ASSETS_PATH = os.path.join(
    os.path.abspath(os.path.join(os.path.join(PROJECT_ROOT),
                                 os.path.pardir, 'fpl_internal')),
    'zooshi', 'rawassets')

# Directory where unprocessed rail flatbuffer data can be found.
RAW_RAIL_PATH = os.path.join(RAW_ASSETS_PATH, 'rails')

# Directory where unprocessed event graph flatbuffer data can be found.
RAW_GRAPH_DEF_PATH = os.path.join(RAW_ASSETS_PATH, 'graphs')

# Directory where unprocessed sound flatbuffer data can be found.
RAW_SOUND_PATH = os.path.join(RAW_ASSETS_PATH, 'sounds')

# Directory where unprocessed sound flatbuffer data can be found.
RAW_SOUND_BANK_PATH = os.path.join(RAW_ASSETS_PATH, 'sound_banks')

# Directory where unprocessed material flatbuffer data can be found.
RAW_MATERIAL_PATH = os.path.join(RAW_ASSETS_PATH, 'materials')

# Directory where unprocessed textures can be found.
TEXTURE_REL_DIR = 'textures'
RAW_TEXTURE_PATH = os.path.join(RAW_ASSETS_PATH, TEXTURE_REL_DIR)

# Directory where unprocessed FBX files can be found.
MESH_REL_DIR = 'meshes'
RAW_MESH_PATH = os.path.join(RAW_ASSETS_PATH, MESH_REL_DIR)

# Directory for textures outside of the main code-base.
INTERNAL_TEXTURE_PATH = os.path.join(INTERNAL_ASSETS_PATH, TEXTURE_REL_DIR)

# Directory where unprocessed FBX files can be found.
INTERNAL_MESH_PATH = os.path.join(INTERNAL_ASSETS_PATH, MESH_REL_DIR)

# Directory where png files are written to before they are converted to webp.
INTERMEDIATE_ASSETS_PATH = os.path.join(PROJECT_ROOT, 'obj', 'assets')

# Directory where png files are written to before they are converted to webp.
INTERMEDIATE_TEXTURE_PATH = os.path.join(INTERMEDIATE_ASSETS_PATH,
                                         TEXTURE_REL_DIR)

# Directory where png files are written to before they are converted to webp.
INTERMEDIATE_MESH_PATH = os.path.join(INTERMEDIATE_ASSETS_PATH, MESH_REL_DIR)

# Directories for animations.
RAW_ANIM_PATH = os.path.join(RAW_ASSETS_PATH, 'anims')
INTERNAL_ANIM_PATH = os.path.join(INTERNAL_ASSETS_PATH, 'anims')

# Directory inside the assets directory where flatbuffer schemas are copied.
SCHEMA_OUTPUT_PATH = 'flatbufferschemas'

# ============================================================================
# The following constants reference data in external components.
# ============================================================================

# Location of flatbuffer schemas in this project.
PROJECT_SCHEMA_PATH = DependencyPath(os.path.join('src', 'flatbufferschemas'))

# A list of json files and their schemas that will be converted to binary files
# by the flatbuffer compiler.
FLATBUFFERS_CONVERSION_DATA = [
    FlatbuffersConversionData(
        schema=PROJECT_SCHEMA_PATH.join('config.fbs'),
        extension='.zooconfig',
        input_files=[os.path.join(RAW_ASSETS_PATH, 'config.json'),]),
    FlatbuffersConversionData(
        schema=PROJECT_SCHEMA_PATH.join('components.fbs'),
        extension='.zooentity',
        input_files=[os.path.join(RAW_ASSETS_PATH, 'entity_prototypes.json'),
                     os.path.join(RAW_ASSETS_PATH, 'entity_rails.json'),
                     os.path.join(RAW_ASSETS_PATH, 'entity_list.json'),
                     os.path.join(RAW_ASSETS_PATH, 'entity_ring.json'),
                     os.path.join(RAW_ASSETS_PATH, 'entity_decorations.json'),
                     os.path.join(RAW_ASSETS_PATH, 'entity_level_0.json')]),
    FlatbuffersConversionData(
        schema=PROJECT_SCHEMA_PATH.join('rail_def.fbs'),
        extension='.rail',
        input_files=glob.glob(os.path.join(RAW_RAIL_PATH, '*.json'))),
    FlatbuffersConversionData(
        schema=PINDROP_ROOT.join('schemas', 'audio_config.fbs'),
        extension='.pinconfig',
        input_files=[os.path.join(RAW_ASSETS_PATH, 'audio_config.json')]),
    FlatbuffersConversionData(
        schema=PROJECT_SCHEMA_PATH.join('input_config.fbs'),
        extension='.zooinconfig',
        input_files=[os.path.join(RAW_ASSETS_PATH, 'input_config.json')]),
    FlatbuffersConversionData(
        schema=PROJECT_SCHEMA_PATH.join('assets.fbs'),
        extension='.zooassets',
        input_files=[os.path.join(RAW_ASSETS_PATH, 'assets.json')]),
    FlatbuffersConversionData(
        schema=PINDROP_ROOT.join('schemas', 'buses.fbs'),
        extension='.pinbus',
        input_files=[os.path.join(RAW_ASSETS_PATH, 'buses.json')]),
    FlatbuffersConversionData(
        schema=PINDROP_ROOT.join('schemas', 'sound_bank_def.fbs'),
        extension='.pinbank',
        input_files=glob.glob(os.path.join(RAW_SOUND_BANK_PATH, '*.json'))),
    FlatbuffersConversionData(
        schema=PINDROP_ROOT.join('schemas', 'sound_collection_def.fbs'),
        extension='.pinsound',
        input_files=glob.glob(os.path.join(RAW_SOUND_PATH, '*.json'))),
    FlatbuffersConversionData(
        schema=FPLBASE_ROOT.join('schemas', 'materials.fbs'),
        extension='.fplmat',
        input_files=glob.glob(os.path.join(RAW_MATERIAL_PATH, '*.json'))),
    FlatbuffersConversionData(
        schema=FPLBASE_ROOT.join('schemas', 'mesh.fbs'),
        extension='.fplmesh',
        input_files=glob.glob(os.path.join(RAW_MESH_PATH, '*.json'))),
    FlatbuffersConversionData(
        schema=PROJECT_SCHEMA_PATH.join('graph.fbs'),
        extension='.bbgraph',
        input_files=glob.glob(os.path.join(RAW_GRAPH_DEF_PATH, '*.json'))),
    # Empty file lists to make other project schemas available to include.
    FlatbuffersConversionData(
        schema=BREADBOARD_ROOT.join('module_library', 'schemas',
                                    'common_modules.fbs'),
        extension='', input_files=[]),
    FlatbuffersConversionData(
        schema=CORGI_ROOT.join('component_library', 'schemas',
                               'bullet_def.fbs'),
        extension='', input_files=[]),
    FlatbuffersConversionData(
        schema=MOTIVE_ROOT.join('schemas', 'anim_table.fbs'),
        extension='', input_files=[]),
    FlatbuffersConversionData(
        schema=SCENE_LAB_ROOT.join('schemas', 'editor_components.fbs'),
        extension='', input_files=[]),
]


def target_file_name(path, target_directory, target_extension):
  """Take the path to a raw png asset and convert it to target webp path.

  Args:
    path: Source file path.
    target_directory: Path to put the target assets.
    target_extension: Extension of target assets.

  Returns:
    Path to the webp file generated from the png file.
  """
  no_path = path.replace(RAW_ASSETS_PATH, '').replace(
      INTERNAL_ASSETS_PATH, '').replace(INTERMEDIATE_ASSETS_PATH, '')
  no_ext = os.path.splitext(no_path)[0]
  target = target_directory + no_ext + '.' + target_extension
  return target


def intermediate_texture_path(path):
  """Take the path to an image and convert it to an intermediate png path.

  Args:
    path: Path to the target assets directory.

  Returns:
    Path to intermediate png file generated from an image path.
  """
  return target_file_name(path, INTERMEDIATE_ASSETS_PATH, 'png')


def processed_texture_path(path, target_directory):
  """Take the path to a raw png asset and convert it to target webp path.

  Args:
    path: Path to the source image file.
    target_directory: Path to the target assets directory.

  Returns:
    Path to processed webp.
  """
  return target_file_name(path, target_directory, 'webp')


def processed_anim_path(path, target_directory):
  """Take the path to a raw anim asset and convert it to target anim path.

  Args:
    path: Path to the source animation file.
    target_directory: Path to the target assets directory.

  Returns:
    Path to output animation file.
  """
  return target_file_name(path, target_directory, 'motiveanim')


def convert_json_to_flatbuffer_binary(flatc, json_path, schema, out_dir,
                                      conversion_data_list):
  """Run the flatbuffer compiler on the given json file and schema.

  Args:
    flatc: Path to the flatc binary.
    json_path: The path to the json file to convert to a flatbuffer binary.
    schema: The path to the schema to use in the conversion process.
    out_dir: The directory to write the flatbuffer binary.
    conversion_data_list: List of FlatbuffersConversionData instances that
      describe all flatbuffers schemas used by the project.  This is used to
      build the list of include directories for all schemas used by the
      project.

  Raises:
    BuildError: Process return code was nonzero.
  """
  command = [flatc, '-o', out_dir]
  for conversion_data in conversion_data_list:
    command.extend(['-I', os.path.dirname(conversion_data.schema.resolve())])
  command.extend(['-b', schema, json_path])
  run_subprocess(command)


def closest_power_of_two(n):
  """Returns the closest power of two (linearly) to n.

  See: http://mccormick.cx/news/entries/nearest-power-of-two

  Args:
    n: Value to find the closest power of two of.

  Returns:
    Closest power of two to "n".
  """
  return pow(2, int(math.log(n, 2) + 0.5))


def next_highest_power_of_two(n):
  """Returns the next highest power of two to n.

  See: http://mccormick.cx/news/entries/nearest-power-of-two

  Args:
    n: Value to finde the next highest power of two of.

  Returns:
    Next highest power of two to "n".
  """
  return int(pow(2, math.ceil(math.log(n, 2))))


class Image(object):
  """Image attributes.

  Attributes:
    filename: Name of the file the image attributes were retrieved from.
    size: (width, height) tuple of the image in pixels.

  Class Attributes:
    CONVERT: Image conversion tool.
    IDENTIFY: Image identification tool.
  """
  USE_GRAPHICSMAGICK = True
  USING_IMAGEMAGICK = False
  # Imagemagick or graphicsmagick convert and identify tools populated when
  # this class is first constructed to delay path resolution.
  CONVERT = ''
  IDENTIFY = ''

  def __init__(self, filename, size, size_upper_bound):
    """Initialize this instance.

    Args:
      filename: Name of the file the image attributes were retrieved from.
      size: (width, height) tuple of the image in pixels.
      size_upper_bound: The maximal dimension of the target image.
    """
    self.__class__.resolve_tool_paths()
    self.filename = filename
    self.size = size
    self.size_upper_bound = size_upper_bound

  @classmethod
  def resolve_tool_paths(cls):
    """Resolve paths to tools if they haven't been resolved already."""
    if not cls.CONVERT or not cls.IDENTIFY:
      graphics_magick = GRAPHICSMAGICK.resolve(raise_on_error=False)
      if cls.USE_GRAPHICSMAGICK and graphics_magick:
        cls.CONVERT = [graphics_magick, 'convert']
        cls.IDENTIFY = [graphics_magick, 'identify']
      else:
        cls.CONVERT = [IMAGEMAGICK_CONVERT.resolve()]
        cls.IDENTIFY = [IMAGEMAGICK_IDENTIFY.resolve()]
        cls.USING_IMAGEMAGICK = True

  @classmethod
  def set_environment(cls):
    """Initialize the environment to execute ImageMagick."""
    # If we're using imagemagick tools.
    if cls.USING_IMAGEMAGICK:
      platform_name = platform.system().lower()
      if platform_name == 'darwin':
        # Need to set DYLD_LIBRARY_PATH for ImageMagick on OSX so it can find
        # shared libraries.
        os.environ['DYLD_LIBRARY_PATH'] = os.path.normpath(os.path.join(
            os.path.dirname(cls.IDENTIFY[0]), os.path.pardir, 'lib'))
      elif platform_name == 'linux':
        # Configure shared library, module, filter and configuration paths.
        libs = os.path.normpath(
            os.path.join(os.path.dirname(cls.IDENTIFY[0]),
                         os.path.pardir, 'lib', 'x86_64-linux-gnu'))
        config_home = glob.glob(os.path.join(libs, 'ImageMagick*'))
        modules_home = (glob.glob(os.path.join(config_home[0], 'modules-*'))
                        if config_home else '')
        os.environ['LD_LIBRARY_PATH'] = libs
        os.environ['MAGICK_CODER_MODULE_PATH'] = (
            os.path.join(modules_home[0], 'coders') if modules_home else '')
        os.environ['MAGICK_CODER_FILTER_PATH'] = (
            os.path.join(modules_home[0], 'filters') if modules_home else '')
        os.environ['MAGICK_CONFIGURE_PATH'] = (
            os.path.join(config_home[0], 'config') if config_home else '')

  @staticmethod
  def read_attributes(filename, size_upper_bound):
    """Read attributes of the image.

    Args:
      filename: Path to the image to query.
      size_upper_bound: The maximal dimension of the target image.

    Returns:
      Image instance containing attributes read from the image.

    Raises:
      BuildError if it's not possible to read the image.
    """
    Image.resolve_tool_paths()
    identify_args = list(Image.IDENTIFY)
    identify_args.extend(['-format', '%w %h', filename])
    Image.set_environment()
    return Image(filename, [int(value) for value in run_subprocess(
        identify_args, capture=True).split()], size_upper_bound)

  def calculate_power_of_two_size(self):
    """Calculate the power of two size of this image.

    Returns:
      (width, height) tuple containing the size of the image rounded to the
      next highest power of 2.
    """
    max_dimension_size = max(self.size)
    if max_dimension_size < self.size_upper_bound:
      return [next_highest_power_of_two(d) for d in self.size]
    scale = float(self.size_upper_bound) / max_dimension_size
    return [closest_power_of_two(d * scale) for d in self.size]

  def convert_resize_image(self, target_image, target_image_size=None):
    """Run the image converter to convert this image.

    The source and target formats are identified by the image extension for
    example if this image references "image.tga" and target_image name is
    "image.png" the image will be converted from a tga format image to png
    format.

    Args:
      target_image: The path to the output image file.
      target_image_size: Size of the output image.  If this isn't specified the
        input image size is used.

    Raises:
      BuildError if the conversion process fails.
    """
    Image.resolve_tool_paths()
    convert_args = list(Image.CONVERT)
    convert_args.append(self.filename)
    if self.size != target_image_size:
      convert_args.extend(['-resize', '%dx%d!' % (target_image_size[0],
                                                  target_image_size[1])])
    convert_args.append(target_image)

    # Output status message.
    source_file = os.path.basename(self.filename)
    target_file = os.path.basename(target_image)
    if self.size != target_image_size:
      source_file += ' (%d, %d)' % (self.size[0], self.size[1])
      target_file += ' (%d, %d)' % (target_image_size[0], target_image_size[1])
    logging.info('Converting %s to %s', source_file, target_file)
    Image.set_environment()
    run_subprocess(convert_args)


def texture_size_upper_bound(filename, meta):
  """Returns the maximum texture size for this filename.

  First checks the metadata file to see if the maximum texture size was
  specified there. If not, return the default maximum size.

  Args:
    filename: Texture file name.
    meta: Metadata object. Queriable on keys with [].

  Returns:
    Upper bound of the texture size.
  """
  meta_upper_bound = meta_value(filename, meta['mesh_meta'], 'texture_size')
  if meta_upper_bound is None:
    return MAX_TGA_SIZE
  return meta_upper_bound


def convert_png_image_to_webp(cwebp, png, out, quality, meta):
  """Run the webp converter on the given png file.

  Args:
    cwebp: Path to the cwebp binary.
    png: The path to the png file to convert into a webp file.
    out: The path of the webp to write to.
    quality: The quality of the processed image, where quality is between 0
        (poor) to 100 (very good). Typical value is around 80.
    meta: Metadata object. Queriable on keys with [].

  Raises:
    BuildError: Process return code was nonzero.
  """
  size_upper_bound = texture_size_upper_bound(png, meta)
  image = Image.read_attributes(png, size_upper_bound)
  target_image_size = image.calculate_power_of_two_size()
  if target_image_size != image.size:
    logging.info('Resizing %s from (%d, %d) to (%d, %d)',
                 png, image.size[0], image.size[1], target_image_size[0],
                 target_image_size[1])

  command = [cwebp, '-resize', str(target_image_size[0]),
             str(target_image_size[1]), '-q', str(quality), png, '-o', out]
  run_subprocess(command)


def convert_fbx_mesh_to_flatbuffer_binary(fbx, target_directory, unit,
                                          texture_formats, recenter,
                                          hierarchy, mesh_relative_dir):
  """Run the mesh_pipeline on the given fbx file.

  Args:
    fbx: The path to the fbx file to convert into a flatbuffer binary.
    target_directory: The path of the flatbuffer binary to write to.
    unit: The unit in the fbx model that you want to be 1 unit in the game.
    texture_formats: String containing of texture formats to pass to the
      mesh_pipeline tool.
    recenter: Whether to recenter the exported mesh at the origin.
    hierarchy: Whether to transform vertices relative to the local pivot of
      each submesh.
    mesh_relative_dir: Directory to place mesh data.

  Raises:
    BuildError: Process return code was nonzero.
  """
  command = [MESH_PIPELINE.resolve(), '--details', '--texture-extension',
             'webp', '--axes', 'z+y+x', '--unit', unit, '--base-dir',
             target_directory, '--relative-dir', mesh_relative_dir]
  if texture_formats is not None:
    command.append('-f')
    command.append(texture_formats)
  if recenter is not None:
    command.append('-c')
  if hierarchy is not None:
    command.append('-h')
  command.append(fbx)
  run_subprocess(command)


def convert_fbx_anim_to_flatbuffer_binary(fbx, repeat, target):
  """Run the anim_pipeline on the given fbx file.

  Args:
    fbx: The path to the fbx file to convert into a flatbuffer binary.
    repeat: Whether the animation should loop / repeat.
    target: The path of the flatbuffer binary to write to.

  Raises:
    BuildError: Process return code was nonzero.
  """
  command = [ANIM_PIPELINE.resolve(), '--details', '--out', target]
  if repeat == 0:
    command.append('--norepeat')
  elif repeat == 1:
    command.append('--repeat')
  command.append(fbx)
  run_subprocess(command)


def needs_rebuild(source, target):
  """Checks if the source file needs to be rebuilt.

  Args:
    source: The source file to be compared.
    target: The target file which we may need to rebuild.

  Returns:
    True if the source file is newer than the target, or if the target file
    does not exist.
  """
  return not os.path.isfile(target) or (
      os.path.getmtime(source) > os.path.getmtime(target))


def processed_mesh_path(path, target_directory):
  """Take the path to an fbx asset and convert it to target mesh path.

  Args:
    path: Path to the source mesh file.
    target_directory: Path to the target assets directory.

  Returns:
    Path to the target mesh.
  """
  return path.replace(RAW_ASSETS_PATH, target_directory).replace(
      INTERNAL_ASSETS_PATH, target_directory).replace('.fbx', '.fplmesh')


def processed_json_path(path, target_directory, target_extension):
  """Take the path to a raw json asset and convert it to target bin path.

  Args:
    path: Path to the source JSON file.
    target_directory: Path to the target assets directory.
    target_extension: Extension of the target file.

  Returns:
    Path to the target file from the source JSON.
  """
  return path.replace(RAW_ASSETS_PATH, target_directory).replace(
      '.json', target_extension)


def fbx_files_to_convert():
  """FBX files to convert to fplmesh."""
  return (glob.glob(os.path.join(RAW_MESH_PATH, '*.fbx')) +
          glob.glob(os.path.join(INTERNAL_MESH_PATH, '*.fbx')))


def texture_files(extension):
  """List of files with `extension` in the texture paths."""
  return (glob.glob(os.path.join(RAW_TEXTURE_PATH, extension)) +
          glob.glob(os.path.join(RAW_MESH_PATH, extension)) +
          glob.glob(os.path.join(INTERNAL_TEXTURE_PATH, extension)) +
          glob.glob(os.path.join(INTERNAL_MESH_PATH, extension)) +
          glob.glob(os.path.join(INTERMEDIATE_TEXTURE_PATH, extension)) +
          glob.glob(os.path.join(INTERMEDIATE_MESH_PATH, extension)))


def png_files_to_convert():
  """PNG files to convert to webp."""
  return texture_files('*.png')


def tga_files_to_convert():
  """TGA files to convert to png."""
  return texture_files('*.tga')


def anim_files_to_convert():
  """FBX files to convert to `.motiveanim`."""
  return (glob.glob(os.path.join(RAW_ANIM_PATH, '*.fbx')) +
          glob.glob(os.path.join(INTERNAL_ANIM_PATH, '*.fbx')))


def meta_value(file_name, meta_table, key):
  """Get metadata value specifed in metadata file.

  Args:
    file_name: File in pipeline.
    meta_table: Metadata file to query. Has key 'name' of partial file names.
    key: Key to query.

  Returns:
    Value associate with the specified key if found, None otherwise.
  """
  for entry in meta_table:
    if entry['name'] in file_name and key in entry:
      return entry[key]
  return None


def generate_mesh_binaries(target_directory, meta):
  """Run the mesh pipeline on the all of the FBX files.

  Args:
    target_directory: Path to the target assets directory.
    meta: Metadata object.
  """
  input_files = fbx_files_to_convert()
  for fbx in input_files:
    target = processed_mesh_path(fbx, target_directory)
    texture_formats = meta_value(fbx, meta['mesh_meta'], 'texture_format')
    recenter = meta_value(fbx, meta['mesh_meta'], 'recenter')
    hierarchy = meta_value(fbx, meta['mesh_meta'], 'hierarchy')
    unit_meta = meta_value(fbx, meta['mesh_meta'], 'unit')
    unit = 'cm' if unit_meta is None else unit_meta
    if needs_rebuild(fbx, target) or needs_rebuild(MESH_PIPELINE.resolve(),
                                                   target):
      convert_fbx_mesh_to_flatbuffer_binary(fbx, target_directory, unit,
                                            texture_formats, recenter,
                                            hierarchy, MESH_REL_DIR)


def generate_anim_binaries(target_directory, meta):
  """Run the mesh pipeline on the all of the FBX files.

  Args:
    target_directory: Path to the target assets directory.
    meta: Metadata object. Queriable on keys with [].
  """
  input_files = anim_files_to_convert()
  for fbx in input_files:
    target = processed_anim_path(fbx, target_directory)
    repeat = meta_value(fbx, meta['anim_meta'], 'repeat')
    if needs_rebuild(fbx, target) or needs_rebuild(ANIM_PIPELINE.resolve(),
                                                   target):
      convert_fbx_anim_to_flatbuffer_binary(fbx, repeat, target)


def generate_flatbuffer_binaries(flatc, target_directory,
                                 conversion_data_list,
                                 schema_output_path):
  """Run the flatbuffer compiler on the all of the flatbuffer json files.

  Args:
    flatc: Path to the flatc binary.
    target_directory: Path to the target assets directory.
    conversion_data_list: List of FlatbuffersConversionData instances that
      reference the JSON files that are going to be converted to binary data
      and their associated schemas.
    schema_output_path: Path to copy schemas to.
  """
  for element in conversion_data_list:
    schema = element.schema.resolve()
    target_schema = os.path.join(target_directory, schema_output_path,
                                 os.path.basename(schema))
    if needs_rebuild(schema, target_schema):
      if not os.path.exists(os.path.dirname(target_schema)):
        os.makedirs(os.path.dirname(target_schema))
      shutil.copy2(schema, target_schema)
    for json_file in element.input_files:
      target = processed_json_path(json_file, target_directory,
                                   element.extension)
      target_file_dir = os.path.dirname(target)
      if not os.path.exists(target_file_dir):
        os.makedirs(target_file_dir)
      if needs_rebuild(json_file, target) or needs_rebuild(schema, target):
        convert_json_to_flatbuffer_binary(
            flatc, json_file, schema, target_file_dir, conversion_data_list)


def generate_png_textures(meta):
  """Run the imange converter to convert tga to png files and resize.

  Args:
    meta: Metadata object. Queriable on keys with [].
  """
  input_files = tga_files_to_convert()
  for tga in input_files:
    out = intermediate_texture_path(tga)
    out_dir = os.path.dirname(out)
    if not os.path.exists(out_dir):
      os.makedirs(out_dir)
    if needs_rebuild(tga, out):
      size_upper_bound = texture_size_upper_bound(tga, meta)
      image = Image.read_attributes(tga, size_upper_bound)
      image.convert_resize_image(
          out, image.calculate_power_of_two_size())


def generate_webp_textures(cwebp, target_directory, meta):
  """Run the webp converter on off of the png files.

  Search for the PNG files lazily, since the mesh pipeline may
  create PNG files that need to be processed.

  Args:
    cwebp: Path to the cwebp binary.
    target_directory: Path to the target assets directory.
    meta: Metadata object. Queriable on keys with [].
  """
  input_files = png_files_to_convert()
  for png in input_files:
    out = processed_texture_path(png, target_directory)
    out_dir = os.path.dirname(out)
    if not os.path.exists(out_dir):
      os.makedirs(out_dir)
    if needs_rebuild(png, out):
      convert_png_image_to_webp(cwebp, png, out, WEBP_QUALITY, meta)


def copy_assets(target_directory):
  """Copy modified assets to the target assets directory.

  All files are copied from ASSETS_PATH to the specified target_directory if
  they're newer than the destination files.

  Args:
    target_directory: Directory to copy assets to.
  """
  assets_dir = target_directory
  source_dir = os.path.realpath(ASSETS_PATH)
  if source_dir != os.path.realpath(assets_dir):
    for dirpath, _, files in os.walk(source_dir):
      for name in files:
        source_filename = os.path.join(dirpath, name)
        relative_source_dir = os.path.relpath(source_filename, source_dir)
        target_dir = os.path.dirname(os.path.join(assets_dir,
                                                  relative_source_dir))
        target_filename = os.path.join(target_dir, name)
        if not os.path.exists(target_dir):
          os.makedirs(target_dir)
        if (not os.path.exists(target_filename) or
            (os.path.getmtime(target_filename) <
             os.path.getmtime(source_filename))):
          shutil.copy2(source_filename, target_filename)


def clean_webp_textures(target_directory):
  """Delete all the processed webp textures.

  Args:
    target_directory: Path to the target assets directory.
  """
  input_files = png_files_to_convert()
  for png in input_files:
    webp = processed_texture_path(png, target_directory)
    if os.path.isfile(webp):
      os.remove(webp)


def clean_flatbuffer_binaries(target_directory):
  """Delete all the processed flatbuffer binaries.

  Args:
    target_directory: Path to the target assets directory.
  """
  for element in FLATBUFFERS_CONVERSION_DATA:
    for json_file in element.input_files:
      path = processed_json_path(json_file, target_directory, element.extension)
      if os.path.isfile(path):
        os.remove(path)


def clean(target_directory):
  """Delete all the processed files."""
  clean_flatbuffer_binaries(target_directory)
  clean_webp_textures(target_directory)


def handle_build_error(error):
  """Prints an error message to stderr for BuildErrors."""
  logging.error('Error running command `%s`. Returned %s.\n%s\n',
                ' '.join(error.argv), str(error.error_code), str(error.message))


def main():
  """Builds or cleans the assets needed for the game.

  To build all assets, either call this script without any arguments. Or
  alternatively, call it with the argument 'all'. To just convert the
  flatbuffer json files, call it with 'flatbuffers'. Likewise to convert the
  png files to webp files, call it with 'webp'. To clean all converted files,
  call it with 'clean'.

  Returns:
    Returns 0 on success.
  """
  # Add resolver for dependency paths.
  DependencyPath.add_resolver(ImageMagickPathResolver(PROJECT_ROOT))

  parser = argparse.ArgumentParser()
  parser.add_argument('--flatc', default=FLATC.resolve(raise_on_error=False),
                      help='Location of the flatbuffers compiler.')
  parser.add_argument('--cwebp', default=CWEBP.resolve(),
                      help='Location of the webp compressor.')
  parser.add_argument('--output', default=ASSETS_PATH,
                      help='Assets output directory.')
  parser.add_argument('--meta', default=ASSET_META,
                      help='File holding metadata for assets.')
  parser.add_argument('args', nargs=argparse.REMAINDER)
  args = parser.parse_args()
  target = args.args[1] if len(args.args) >= 2 else 'all'
  if target not in ('all', 'png', 'mesh', 'anim', 'flatbuffers', 'webp',
                    'clean'):
    sys.stderr.write('No rule to build target %s.\n' % target)

  # Load the json file that holds asset metadata.
  meta_file = open(args.meta, 'r')
  meta = json.load(meta_file)

  if target != 'clean':
    copy_assets(args.output)
  # The mesh pipeline must run before the webp texture converter,
  # since the mesh pipeline might create textures that need to be
  # converted.
  try:
    if target in ('all', 'png'):
      generate_png_textures(meta)
    if target in ('all', 'mesh'):
      generate_mesh_binaries(args.output, meta)
    if target in ('all', 'anim'):
      generate_anim_binaries(args.output, meta)
    if target in ('all', 'flatbuffers'):
      generate_flatbuffer_binaries(args.flatc, args.output,
                                   FLATBUFFERS_CONVERSION_DATA,
                                   SCHEMA_OUTPUT_PATH)
    if target in ('all', 'webp'):
      generate_webp_textures(args.cwebp, args.output, meta)
    if target == 'clean':
      try:
        clean(args.output)
      except OSError as error:
        sys.stderr.write('Error cleaning: %s' % str(error))
        return 1
  except BuildError as error:
    handle_build_error(error)
    return 1
  except DependencyPathError as error:
    logging.error('%s not found in %s', error.filename, str(error.paths))
    return 1

  return 0


if __name__ == '__main__':
  sys.exit(main())
