#pragma once

#ifndef _COMMON_MANAGER_H
#define _COMMON_MANAGER_H

#include <List>

class Entity;
class Component;

enum class ManID
{
	Physics = 0
};

class Manager {
protected:
	std::list<Component*> _compsList;
	ManID _manId;
public:
	Manager(ManID id);
	virtual ~Manager();

	virtual void addComponent(Entity* ent, int compId) = 0;
	virtual void start() = 0;
	virtual void update() = 0;

	std::list<Component*> getComponents();
	virtual void destroyAllComponents();
	virtual bool destroyComponent(Entity* ent, int compId);

	int getId();
};

#endif