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


import distutils.dir_util
import glob
import json
import os
import sys
# The project root directory, which is two levels up from this script's
# directory.
PROJECT_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__),
                                            os.path.pardir))
# Add possible paths that the scene_lab_assets_builder module might be in.
sys.path.append(os.path.join(PROJECT_ROOT, os.path.pardir, os.path.pardir,
                             'libs', 'scene_lab', 'scripts'))
sys.path.append(os.path.join(PROJECT_ROOT, 'dependencies',
                             'scene_lab', 'scripts'))
import scene_lab_asset_builder as builder

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
RAW_TEXTURE_PATH = os.path.join(RAW_ASSETS_PATH, 'textures')

# Directory where unprocessed FBX files can be found.
RAW_MESH_PATH = os.path.join(RAW_ASSETS_PATH, 'meshes')

# Directory where png files are written to before they are converted to webp.
INTERMEDIATE_ASSETS_PATH = os.path.join(PROJECT_ROOT, 'obj', 'assets')

# Directory where png files are written to before they are converted to webp.
INTERMEDIATE_TEXTURE_PATH = os.path.join(INTERMEDIATE_ASSETS_PATH, 'textures')

# Directories for animations.
RAW_ANIM_PATH = os.path.join(RAW_ASSETS_PATH, 'anims')

# Directory inside the assets directory where flatbuffer schemas are copied.
SCHEMA_OUTPUT_PATH = 'flatbufferschemas'

# Potential root directories for source assets.
ASSET_ROOTS = [RAW_ASSETS_PATH, INTERMEDIATE_TEXTURE_PATH]
# Overlay directories.
OVERLAY_DIRS = [os.path.relpath(f, RAW_ASSETS_PATH)
                for f in glob.glob(os.path.join(RAW_ASSETS_PATH, 'overlays',
                                                '*'))]

# ============================================================================
# The following constants reference data in external components.
# ============================================================================

# Location of flatbuffer schemas in this project.
PROJECT_SCHEMA_PATH = builder.DependencyPath(os.path.join('src', 'flatbufferschemas'))

# A list of json files and their schemas that will be converted to binary files
# by the flatbuffer compiler.
FLATBUFFERS_CONVERSION_DATA = [
    builder.FlatbuffersConversionData(
        schema=PROJECT_SCHEMA_PATH.join('config.fbs'),
        extension='zooconfig',
        input_files=[os.path.join(RAW_ASSETS_PATH, 'config.json'),]),
    builder.FlatbuffersConversionData(
        schema=PROJECT_SCHEMA_PATH.join('components.fbs'),
        extension='zooentity',
        input_files=[os.path.join(RAW_ASSETS_PATH, 'entity_prototypes.json'),
                     os.path.join(RAW_ASSETS_PATH, 'entity_rails.json'),
                     os.path.join(RAW_ASSETS_PATH, 'entity_list.json'),
                     os.path.join(RAW_ASSETS_PATH, 'entity_ring.json'),
                     os.path.join(RAW_ASSETS_PATH, 'entity_decorations.json'),
                     os.path.join(RAW_ASSETS_PATH, 'entity_level_0.json')]),
    builder.FlatbuffersConversionData(
        schema=PROJECT_SCHEMA_PATH.join('rail_def.fbs'),
        extension='rail',
        input_files=glob.glob(os.path.join(RAW_RAIL_PATH, '*.json'))),
    builder.FlatbuffersConversionData(
        schema=builder.PINDROP_ROOT.join('schemas', 'audio_config.fbs'),
        extension='pinconfig',
        input_files=[os.path.join(RAW_ASSETS_PATH, 'audio_config.json')]),
    builder.FlatbuffersConversionData(
        schema=PROJECT_SCHEMA_PATH.join('input_config.fbs'),
        extension='zooinconfig',
        input_files=[os.path.join(RAW_ASSETS_PATH, 'input_config.json')]),
    builder.FlatbuffersConversionData(
        schema=PROJECT_SCHEMA_PATH.join('assets.fbs'),
        extension='zooassets',
        input_files=[os.path.join(RAW_ASSETS_PATH, 'assets.json')]),
    builder.FlatbuffersConversionData(
        schema=builder.PINDROP_ROOT.join('schemas', 'buses.fbs'),
        extension='pinbus',
        input_files=[os.path.join(RAW_ASSETS_PATH, 'buses.json')]),
    builder.FlatbuffersConversionData(
        schema=builder.PINDROP_ROOT.join('schemas', 'sound_bank_def.fbs'),
        extension='pinbank',
        input_files=glob.glob(os.path.join(RAW_SOUND_BANK_PATH, '*.json'))),
    builder.FlatbuffersConversionData(
        schema=builder.PINDROP_ROOT.join('schemas', 'sound_collection_def.fbs'),
        extension='pinsound',
        input_files=glob.glob(os.path.join(RAW_SOUND_PATH, '*.json'))),
    builder.FlatbuffersConversionData(
        schema=builder.FPLBASE_ROOT.join('schemas', 'materials.fbs'),
        extension='fplmat',
        input_files=glob.glob(os.path.join(RAW_MATERIAL_PATH, '*.json'))),
    builder.FlatbuffersConversionData(
        schema=builder.FPLBASE_ROOT.join('schemas', 'mesh.fbs'),
        extension='fplmesh',
        input_files=glob.glob(os.path.join(RAW_MESH_PATH, '*.json'))),
    builder.FlatbuffersConversionData(
        schema=PROJECT_SCHEMA_PATH.join('graph.fbs'),
        extension='bbgraph',
        input_files=glob.glob(os.path.join(RAW_GRAPH_DEF_PATH, '*.json'))),
    # Empty file lists to make other project schemas available to include.
    builder.FlatbuffersConversionData(
        schema=builder.BREADBOARD_ROOT.join('module_library', 'schemas',
                                    'common_modules.fbs'),
        extension='', input_files=[]),
    builder.FlatbuffersConversionData(
        schema=builder.CORGI_ROOT.join('component_library', 'schemas',
                               'bullet_def.fbs'),
        extension='', input_files=[]),
    builder.FlatbuffersConversionData(
        schema=builder.MOTIVE_ROOT.join('schemas', 'anim_table.fbs'),
        extension='', input_files=[]),
    builder.FlatbuffersConversionData(
        schema=builder.SCENE_LAB_ROOT.join('schemas', 'editor_components.fbs'),
        extension='', input_files=[]),
]

def fbx_files_to_convert():
  """FBX files to convert to fplmesh."""
  return glob.glob(os.path.join(RAW_MESH_PATH, '*.fbx'))


def texture_files(pattern):
  """List of files matching pattern in the texture paths.

  Args:
    pattern: glob style pattern.

  Returns:
    List of filenames.
  """
  return (glob.glob(os.path.join(RAW_TEXTURE_PATH, pattern)) +
          glob.glob(os.path.join(RAW_MESH_PATH, pattern)) +
          glob.glob(os.path.join(INTERMEDIATE_TEXTURE_PATH, pattern)))


def png_files_to_convert():
  """PNG files to convert to webp."""
  return texture_files('*.png')


def tga_files_to_convert():
  """TGA files to convert to png."""
  return texture_files('*.tga')


def anim_files_to_convert():
  """FBX files to convert to `.motiveanim`."""
  return glob.glob(os.path.join(RAW_ANIM_PATH, '*.fbx'))


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
  builder.DependencyPath.add_resolver(
      builder.ImageMagickPathResolver(PROJECT_ROOT))

  parser = builder.argparse.ArgumentParser()
  parser.add_argument('--flatc',
                      default=builder.FLATC.resolve(raise_on_error=False),
                      help='Location of the flatbuffers compiler.')
  parser.add_argument('--cwebp', default=builder.CWEBP.resolve(),
                      help='Location of the webp compressor.')
  parser.add_argument('--anim-pipeline',
                      default=builder.ANIM_PIPELINE.resolve(),
                      help='Location of the anim_pipeline tool.')
  parser.add_argument('--mesh-pipeline',
                      default=builder.MESH_PIPELINE.resolve(),
                      help='Location of the mesh_pipeline tool.')
  parser.add_argument('--output', default=ASSETS_PATH,
                      help='Assets output directory.')
  parser.add_argument('--meta', default=ASSET_META,
                      help='File holding metadata for assets.')
  parser.add_argument('--max_texture_size', default=builder.MAX_TEXTURE_SIZE,
                      help='Max texture size in pixels along the largest '
                      'dimension')
  parser.add_argument('-v', '--verbose', help='Display verbose output.',
                      action='store_true')
  parser.add_argument('args', nargs=builder.argparse.REMAINDER)
  args = parser.parse_args()
  target = args.args[0] if len(args.args) >= 1 else 'all'

  if target not in ('all', 'png', 'mesh', 'anim', 'flatbuffers', 'webp',
                    'clean'):
    sys.stderr.write('No rule to build target %s.\n' % target)

  builder.logging.basicConfig(format='%(message)s')
  builder.logging.getLogger().setLevel(
      builder.logging.DEBUG if args.verbose else builder.logging.INFO)

  # Load the json file that holds asset metadata.
  meta_file = open(args.meta, 'r')
  meta = json.load(meta_file)

  if target != 'clean':
    distutils.dir_util.copy_tree(ASSETS_PATH, args.output)
  # The mesh pipeline must run before the webp texture converter,
  # since the mesh pipeline might create textures that need to be
  # converted.
  try:
    if target in ('all', 'png'):
      builder.generate_png_textures(
          builder.input_files_add_overlays(tga_files_to_convert(), ASSET_ROOTS,
                                           OVERLAY_DIRS,
                                           overlay_globs=['*.tga']),
          ASSET_ROOTS, INTERMEDIATE_TEXTURE_PATH, meta, args.max_texture_size)

    if target in ('all', 'mesh'):
      builder.generate_mesh_binaries(
          args.mesh_pipeline,
          builder.input_files_add_overlays(fbx_files_to_convert(), ASSET_ROOTS,
                                           OVERLAY_DIRS,
                                           overlay_globs=['*.fbx']),
          ASSET_ROOTS, args.output, meta)

    if target in ('all', 'anim'):
      builder.generate_anim_binaries(
          args.anim_pipeline,
          builder.input_files_add_overlays(anim_files_to_convert(), ASSET_ROOTS,
                                           OVERLAY_DIRS,
                                           overlay_globs=['*.fbx']),
          ASSET_ROOTS, args.output, meta)

    if target in ('all', 'flatbuffers'):
      builder.generate_flatbuffer_binaries(
          args.flatc, builder.flatbuffers_conversion_data_add_overlays(
              FLATBUFFERS_CONVERSION_DATA, ASSET_ROOTS, OVERLAY_DIRS),
          ASSET_ROOTS, args.output, SCHEMA_OUTPUT_PATH)

    if target in ('all', 'webp'):
      builder.generate_webp_textures(
          args.cwebp, builder.input_files_add_overlays(
              png_files_to_convert(), ASSET_ROOTS, OVERLAY_DIRS,
              overlay_globs=['*.png']),
          ASSET_ROOTS, args.output, meta, args.max_texture_size)

    if target == 'clean':
      try:
        builder.clean(INTERMEDIATE_TEXTURE_PATH, ASSET_ROOTS, [],
                      {'png': builder.input_files_add_overlays(
                          tga_files_to_convert(), ASSET_ROOTS, OVERLAY_DIRS,
                          overlay_globs=['*.tga'])})
        builder.clean(
            args.output, ASSET_ROOTS,
            builder.flatbuffers_conversion_data_add_overlays(
                FLATBUFFERS_CONVERSION_DATA, ASSET_ROOTS, OVERLAY_DIRS),
            {'webp': builder.input_files_add_overlays(
                png_files_to_convert(), ASSET_ROOTS, OVERLAY_DIRS,
                overlay_globs=['*.png']),
             'motiveanim': builder.input_files_add_overlays(
                 anim_files_to_convert(), ASSET_ROOTS, OVERLAY_DIRS,
                 overlay_globs=['*.fbx']),
             'fplmesh': builder.input_files_add_overlays(
                 fbx_files_to_convert(), ASSET_ROOTS, OVERLAY_DIRS,
                 overlay_globs=['*.fbx'])})
      except OSError as error:
        sys.stderr.write('Error cleaning: %s' % str(error))
        return 1

  except builder.BuildError as error:
    builder.logging.error('Error running command `%s`. Returned %s.\n%s\n',
                          ' '.join(error.argv), str(error.error_code),
                          str(error.message))
    return 1
  except builder.DependencyPathError as error:
    builder.logging.error('%s not found in %s', error.filename,
                          str(error.paths))
    return 1

  return 0


if __name__ == '__main__':
  sys.exit(main())
