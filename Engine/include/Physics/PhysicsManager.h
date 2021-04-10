#pragma once

class btDefaultCollisionConfiguration;
class btCollisionDispatcher;
class btBroadphaseInterface;
class btSequentialImpulseConstraintSolver;
class btDiscreteDynamicsWorld;
class btRigidBody;
class Vector3;

class PhysicsManager
{
private:
	static PhysicsManager* instance;

	//Configuracion sobre la gestion de colisiones con bullet, nosotros usaremos la configuracion por defecto
	btDefaultCollisionConfiguration* collConfig;

	//Variable de bullet que hace de "pasador" de colisiones
	btCollisionDispatcher* collDispatcher;

	//Variable de bullet que se usa para hacer calculos de manera eficiente para generar posibles colisiones
	btBroadphaseInterface* broadPhaseInterface;

	//Variable de bullet que hace de solucionador de restricciones 
	btSequentialImpulseConstraintSolver* constraintSolver;

	//Variable de bullet a la que se le pasa todas las variables anteriores como configuracion de la fisica
	btDiscreteDynamicsWorld* dynamicsWorld;

	//estoy hay que hablarlo no lo tengo muy claro
	/*OgreDebugDrawer* mDebugDrawer_ = nullptr;

	std::vector<btRigidBody*> ribs_;
	std::vector<btBoxShape*> shapes_;
	std::vector<btMotionState*> states_;*/

	PhysicsManager();
	~PhysicsManager();

public:
	//metodo static de init para inicializar la instancia del PhysicsManager
	static void init();

	//nos devuelve la instancia
	static PhysicsManager* getInstance();

	//inicializa todas las variables fisicas asi como el "mundo" a partir de dichas variables
	void init(const Vector3 gravity);

	//destruye todas las variables relacionadas con la fisica
	void destroyWorld();

	//destruye el mundo fisico
	void destroyWorldContent();

	void destroyRigidBody(btRigidBody* body);

	//actualiza la simulacion fisica
	void updatePhys();

	btDiscreteDynamicsWorld* getWorld() const;

	//hay que hablar esto
	btRigidBody* createRB(Vector3 pos, Vector3 shape, float mass);

};

