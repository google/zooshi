# Copyright 2015 Google Inc. All rights reserved.
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

LOCAL_PATH:=$(call my-dir)

# Project directory relative to this file.
ZOOSHI_DIR:=$(LOCAL_PATH)/../../..
include $(ZOOSHI_DIR)/jni/android_config.mk

LOCAL_PATH:=$(DEPENDENCIES_BULLETPHYSICS_DIR)/src

LOCAL_MODULE := libbullet
LOCAL_MODULE_FILENAME := libbullet

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/ \
    $(LOCAL_PATH)/BulletCollision/BroadphaseCollision \
    $(LOCAL_PATH)/BulletCollision/CollisionDispatch \
    $(LOCAL_PATH)/BulletCollision/CollisionShapes \
    $(LOCAL_PATH)/BulletCollision/NarrowPhaseCollision \
    $(LOCAL_PATH)/BulletDynamics/ConstraintSolver \
    $(LOCAL_PATH)/BulletDynamics/Dynamics \
    $(LOCAL_PATH)/BulletDynamics/Vehicle \
    $(LOCAL_PATH)/LinearMath \

LOCAL_ARM_MODE := arm

BULLET_COLLISION_SRC_FILES := \
    BulletCollision/BroadphaseCollision/btAxisSweep3.cpp \
    BulletCollision/BroadphaseCollision/btBroadphaseProxy.cpp \
    BulletCollision/BroadphaseCollision/btCollisionAlgorithm.cpp \
    BulletCollision/BroadphaseCollision/btDbvt.cpp \
    BulletCollision/BroadphaseCollision/btDbvtBroadphase.cpp \
    BulletCollision/BroadphaseCollision/btDispatcher.cpp \
    BulletCollision/BroadphaseCollision/btMultiSapBroadphase.cpp \
    BulletCollision/BroadphaseCollision/btOverlappingPairCache.cpp \
    BulletCollision/BroadphaseCollision/btQuantizedBvh.cpp \
    BulletCollision/BroadphaseCollision/btSimpleBroadphase.cpp \
    BulletCollision/CollisionDispatch/btActivatingCollisionAlgorithm.cpp \
    BulletCollision/CollisionDispatch/btBoxBoxCollisionAlgorithm.cpp \
    BulletCollision/CollisionDispatch/btBox2dBox2dCollisionAlgorithm.cpp \
    BulletCollision/CollisionDispatch/btBoxBoxDetector.cpp \
    BulletCollision/CollisionDispatch/btCollisionDispatcher.cpp \
    BulletCollision/CollisionDispatch/btCollisionObject.cpp \
    BulletCollision/CollisionDispatch/btCollisionWorld.cpp \
    BulletCollision/CollisionDispatch/btCollisionWorldImporter.cpp \
    BulletCollision/CollisionDispatch/btCompoundCollisionAlgorithm.cpp \
    BulletCollision/CollisionDispatch/btCompoundCompoundCollisionAlgorithm.cpp \
    BulletCollision/CollisionDispatch/btConvexConcaveCollisionAlgorithm.cpp \
    BulletCollision/CollisionDispatch/btConvexConvexAlgorithm.cpp \
    BulletCollision/CollisionDispatch/btConvexPlaneCollisionAlgorithm.cpp \
    BulletCollision/CollisionDispatch/btConvex2dConvex2dAlgorithm.cpp \
    BulletCollision/CollisionDispatch/btDefaultCollisionConfiguration.cpp \
    BulletCollision/CollisionDispatch/btEmptyCollisionAlgorithm.cpp \
    BulletCollision/CollisionDispatch/btGhostObject.cpp \
    BulletCollision/CollisionDispatch/btHashedSimplePairCache.cpp \
    BulletCollision/CollisionDispatch/btInternalEdgeUtility.cpp \
    BulletCollision/CollisionDispatch/btManifoldResult.cpp \
    BulletCollision/CollisionDispatch/btSimulationIslandManager.cpp \
    BulletCollision/CollisionDispatch/btSphereBoxCollisionAlgorithm.cpp \
    BulletCollision/CollisionDispatch/btSphereSphereCollisionAlgorithm.cpp \
    BulletCollision/CollisionDispatch/btSphereTriangleCollisionAlgorithm.cpp \
    BulletCollision/CollisionDispatch/btUnionFind.cpp \
    BulletCollision/CollisionDispatch/SphereTriangleDetector.cpp \
    BulletCollision/CollisionShapes/btBoxShape.cpp \
    BulletCollision/CollisionShapes/btBox2dShape.cpp \
    BulletCollision/CollisionShapes/btBvhTriangleMeshShape.cpp \
    BulletCollision/CollisionShapes/btCapsuleShape.cpp \
    BulletCollision/CollisionShapes/btCollisionShape.cpp \
    BulletCollision/CollisionShapes/btCompoundShape.cpp \
    BulletCollision/CollisionShapes/btConcaveShape.cpp \
    BulletCollision/CollisionShapes/btConeShape.cpp \
    BulletCollision/CollisionShapes/btConvexHullShape.cpp \
    BulletCollision/CollisionShapes/btConvexInternalShape.cpp \
    BulletCollision/CollisionShapes/btConvexPointCloudShape.cpp \
    BulletCollision/CollisionShapes/btConvexPolyhedron.cpp \
    BulletCollision/CollisionShapes/btConvexShape.cpp \
    BulletCollision/CollisionShapes/btConvex2dShape.cpp \
    BulletCollision/CollisionShapes/btConvexTriangleMeshShape.cpp \
    BulletCollision/CollisionShapes/btCylinderShape.cpp \
    BulletCollision/CollisionShapes/btEmptyShape.cpp \
    BulletCollision/CollisionShapes/btHeightfieldTerrainShape.cpp \
    BulletCollision/CollisionShapes/btMinkowskiSumShape.cpp \
    BulletCollision/CollisionShapes/btMultimaterialTriangleMeshShape.cpp \
    BulletCollision/CollisionShapes/btMultiSphereShape.cpp \
    BulletCollision/CollisionShapes/btOptimizedBvh.cpp \
    BulletCollision/CollisionShapes/btPolyhedralConvexShape.cpp \
    BulletCollision/CollisionShapes/btScaledBvhTriangleMeshShape.cpp \
    BulletCollision/CollisionShapes/btShapeHull.cpp \
    BulletCollision/CollisionShapes/btSphereShape.cpp \
    BulletCollision/CollisionShapes/btStaticPlaneShape.cpp \
    BulletCollision/CollisionShapes/btStridingMeshInterface.cpp \
    BulletCollision/CollisionShapes/btTetrahedronShape.cpp \
    BulletCollision/CollisionShapes/btTriangleBuffer.cpp \
    BulletCollision/CollisionShapes/btTriangleCallback.cpp \
    BulletCollision/CollisionShapes/btTriangleIndexVertexArray.cpp \
    BulletCollision/CollisionShapes/btTriangleIndexVertexMaterialArray.cpp \
    BulletCollision/CollisionShapes/btTriangleMesh.cpp \
    BulletCollision/CollisionShapes/btTriangleMeshShape.cpp \
    BulletCollision/CollisionShapes/btUniformScalingShape.cpp \
    BulletCollision/Gimpact/btContactProcessing.cpp \
    BulletCollision/Gimpact/btGenericPoolAllocator.cpp \
    BulletCollision/Gimpact/btGImpactBvh.cpp \
    BulletCollision/Gimpact/btGImpactCollisionAlgorithm.cpp \
    BulletCollision/Gimpact/btGImpactQuantizedBvh.cpp \
    BulletCollision/Gimpact/btGImpactShape.cpp \
    BulletCollision/Gimpact/btTriangleShapeEx.cpp \
    BulletCollision/Gimpact/gim_box_set.cpp \
    BulletCollision/Gimpact/gim_contact.cpp \
    BulletCollision/Gimpact/gim_memory.cpp \
    BulletCollision/Gimpact/gim_tri_collision.cpp \
    BulletCollision/NarrowPhaseCollision/btContinuousConvexCollision.cpp \
    BulletCollision/NarrowPhaseCollision/btConvexCast.cpp \
    BulletCollision/NarrowPhaseCollision/btGjkConvexCast.cpp \
    BulletCollision/NarrowPhaseCollision/btGjkEpa2.cpp \
    BulletCollision/NarrowPhaseCollision/btGjkEpaPenetrationDepthSolver.cpp \
    BulletCollision/NarrowPhaseCollision/btGjkPairDetector.cpp \
    BulletCollision/NarrowPhaseCollision/btMinkowskiPenetrationDepthSolver.cpp \
    BulletCollision/NarrowPhaseCollision/btPersistentManifold.cpp \
    BulletCollision/NarrowPhaseCollision/btRaycastCallback.cpp \
    BulletCollision/NarrowPhaseCollision/btSubSimplexConvexCast.cpp \
    BulletCollision/NarrowPhaseCollision/btVoronoiSimplexSolver.cpp \
    BulletCollision/NarrowPhaseCollision/btPolyhedralContactClipping.cpp

BULLET_DYNAMICS_SRC_FILES := \
    BulletDynamics/Character/btKinematicCharacterController.cpp \
    BulletDynamics/ConstraintSolver/btConeTwistConstraint.cpp \
    BulletDynamics/ConstraintSolver/btContactConstraint.cpp \
    BulletDynamics/ConstraintSolver/btFixedConstraint.cpp \
    BulletDynamics/ConstraintSolver/btGearConstraint.cpp \
    BulletDynamics/ConstraintSolver/btGeneric6DofConstraint.cpp \
    BulletDynamics/ConstraintSolver/btGeneric6DofSpringConstraint.cpp \
    BulletDynamics/ConstraintSolver/btGeneric6DofSpring2Constraint.cpp \
    BulletDynamics/ConstraintSolver/btHinge2Constraint.cpp \
    BulletDynamics/ConstraintSolver/btHingeConstraint.cpp \
    BulletDynamics/ConstraintSolver/btPoint2PointConstraint.cpp \
    BulletDynamics/ConstraintSolver/btSequentialImpulseConstraintSolver.cpp \
    BulletDynamics/ConstraintSolver/btNNCGConstraintSolver.cpp \
    BulletDynamics/ConstraintSolver/btSliderConstraint.cpp \
    BulletDynamics/ConstraintSolver/btSolve2LinearConstraint.cpp \
    BulletDynamics/ConstraintSolver/btTypedConstraint.cpp \
    BulletDynamics/ConstraintSolver/btUniversalConstraint.cpp \
    BulletDynamics/Dynamics/btDiscreteDynamicsWorld.cpp \
    BulletDynamics/Dynamics/btRigidBody.cpp \
    BulletDynamics/Dynamics/btSimpleDynamicsWorld.cpp \
    BulletDynamics/Vehicle/btRaycastVehicle.cpp \
    BulletDynamics/Vehicle/btWheelInfo.cpp \
    BulletDynamics/Featherstone/btMultiBody.cpp \
    BulletDynamics/Featherstone/btMultiBodyConstraintSolver.cpp \
    BulletDynamics/Featherstone/btMultiBodyDynamicsWorld.cpp \
    BulletDynamics/Featherstone/btMultiBodyJointLimitConstraint.cpp \
    BulletDynamics/Featherstone/btMultiBodyConstraint.cpp \
    BulletDynamics/Featherstone/btMultiBodyPoint2Point.cpp \
    BulletDynamics/Featherstone/btMultiBodyJointMotor.cpp \
    BulletDynamics/MLCPSolvers/btDantzigLCP.cpp \
    BulletDynamics/MLCPSolvers/btMLCPSolver.cpp \
    BulletDynamics/MLCPSolvers/btLemkeAlgorithm.cpp

BULLET_LINEARMATH_SRC_FILES := \
    LinearMath/btAlignedAllocator.cpp \
    LinearMath/btConvexHull.cpp \
    LinearMath/btConvexHullComputer.cpp \
    LinearMath/btGeometryUtil.cpp \
    LinearMath/btPolarDecomposition.cpp \
    LinearMath/btQuickprof.cpp \
    LinearMath/btSerializer.cpp \
    LinearMath/btVector3.cpp

LOCAL_SRC_FILES := \
    $(BULLET_COLLISION_SRC_FILES) \
    $(BULLET_DYNAMICS_SRC_FILES) \
    $(BULLET_LINEARMATH_SRC_FILES)

include $(BUILD_STATIC_LIBRARY)
