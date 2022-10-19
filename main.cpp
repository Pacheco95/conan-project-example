#include <cctype>

#include <PxConfig.h>
#include <PxPhysicsAPI.h>

#include <plog/Log.h>
#include <plog/Appenders/ColorConsoleAppender.h>


#define PX_RELEASE(x) if(x) { x->release(); x = NULL; }

using namespace physx;
using namespace std;

constexpr auto PVD_HOST = "127.0.0.1";

PxDefaultAllocator gAllocator;
PxDefaultErrorCallback gErrorCallback;

PxFoundation *gFoundation = nullptr;
PxPhysics *gPhysics = nullptr;

PxDefaultCpuDispatcher *gDispatcher = nullptr;
PxScene *gScene = nullptr;

PxMaterial *gMaterial = nullptr;

PxPvd *gPvd = nullptr;

PxReal stackZ = 10.0f;

void configureLogger() {
    static plog::ColorConsoleAppender<plog::TxtFormatter> appender;
    plog::init(plog::debug, &appender);
}


PxRigidDynamic *createDynamic(const PxTransform &t, const PxGeometry &geometry, const PxVec3 &velocity = PxVec3(0)) {
    PxRigidDynamic *dynamic = PxCreateDynamic(*gPhysics, t, geometry, *gMaterial, 10.0f);
    dynamic->setAngularDamping(0.5f);
    dynamic->setLinearVelocity(velocity);
    gScene->addActor(*dynamic);
    return dynamic;
}

void createStack(const PxTransform &t, PxU32 size, PxReal halfExtent) {
    PxShape *shape = gPhysics->createShape(PxBoxGeometry(halfExtent, halfExtent, halfExtent), *gMaterial);
    for (PxU32 i = 0; i < size; i++) {
        for (PxU32 j = 0; j < size - i; j++) {
            PxTransform localTm(PxVec3(PxReal(j *
            2) -PxReal(size - i), PxReal(i * 2 + 1), 0) *halfExtent);
            PxRigidDynamic *body = gPhysics->createRigidDynamic(t.transform(localTm));
            body->attachShape(*shape);
            PxRigidBodyExt::updateMassAndInertia(*body, 10.0f);
            gScene->addActor(*body);
        }
    }
    shape->release();
}

void initPhysics(bool interactive) {
    gFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, gAllocator, gErrorCallback);

    gPvd = PxCreatePvd(*gFoundation);
    PxPvdTransport *transport = PxDefaultPvdSocketTransportCreate(PVD_HOST, 5425, 10);
    gPvd->connect(*transport, PxPvdInstrumentationFlag::eALL);

    gPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *gFoundation, PxTolerancesScale(), true, gPvd);

    PxSceneDesc sceneDesc(gPhysics->getTolerancesScale());
    sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
    gDispatcher = PxDefaultCpuDispatcherCreate(2);
    sceneDesc.cpuDispatcher = gDispatcher;
    sceneDesc.filterShader = PxDefaultSimulationFilterShader;
    gScene = gPhysics->createScene(sceneDesc);

    PxPvdSceneClient *pvdClient = gScene->getScenePvdClient();
    if (pvdClient) {
        pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
        pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
        pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
    }
    gMaterial = gPhysics->createMaterial(0.5f, 0.5f, 0.6f);

    PxRigidStatic *groundPlane = PxCreatePlane(*gPhysics, PxPlane(0, 1, 0, 0), *gMaterial);
    gScene->addActor(*groundPlane);

    for (PxU32 i = 0; i < 5; i++)
        createStack(PxTransform(PxVec3(0, 0, stackZ -= 10.0f)), 10, 2.0f);

    if (!interactive)
        createDynamic(PxTransform(PxVec3(0, 40, 100)), PxSphereGeometry(10), PxVec3(0, -50, -100));
}

void stepPhysics(bool /*interactive*/) {
    gScene->simulate(1.0f / 60.0f);
    gScene->fetchResults(true);
}

void cleanupPhysics(bool /*interactive*/) {
    PX_RELEASE(gScene);
    PX_RELEASE(gDispatcher);
    PX_RELEASE(gPhysics);
    if (gPvd) {
        PxPvdTransport *transport = gPvd->getTransport();
        gPvd->release();
        gPvd = nullptr;
        PX_RELEASE(transport);
    }
    PX_RELEASE(gFoundation);
}


int main() {
    LOGI << "You can download NVIDIA PVD at https://developer.nvidia.com/physx-visual-debugger to see the simulation";

    configureLogger();

    LOGD << "Setting up world configuration...";

    const PxU32 frameCount = 2500;
    initPhysics(false);

    LOGD << "Simulation started";

    for (PxU32 i = 0; i < frameCount; i++)
        stepPhysics(false);

    LOGD << "Simulation finished";

    cleanupPhysics(false);

    LOGD << "Resources cleaned";

    return 0;
}
