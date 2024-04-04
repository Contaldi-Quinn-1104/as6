#include "RadiansDegrees.hpp"
#include "Vector4.hpp"
#include "raylib-cpp.hpp"
#include <memory>
#include <ranges>
#include <iostream>
#include <concepts>
#include "BufferedInput.hpp"
#include <vector>
#include "raylib.h"
#include "skybox.hpp"

template<typename T>
concept Transformer = requires(T t, raylib::Transform m) {
	{ t.operator()(m) } -> std::convertible_to<raylib::Transform>;
};

struct Entity; // Forward declaration

// Component class which will be used to create all the components
struct Component
{
    struct Entity* object;
	Component(struct Entity& e) : object(&e) { }
    virtual void setup() { };
    virtual void cleanup() { };
    virtual void tick(float dt) { };
};

struct TransformComponent : public Component
{  
    using Component::Component;
    raylib::Vector3 position = {0,0,0};
    float scale_X = 1.0f;
    float scale_Y = 1.0f;
    float scale_Z = 1.0f;
    raylib::Quaternion rotation = raylib::Quaternion::Identity();
    raylib::Degree start_RotationX= 0;
    raylib::Degree start_RotationY= 0;
    raylib::Degree start_RotationZ= 0;
    raylib::Degree heading = 0;
    TransformComponent(Entity& e, raylib::Vector3 initialPosition, raylib::Quaternion initialRotation) : Component(e), position(initialPosition), rotation(initialRotation) { }
};

struct Entity 
{
	std::vector<std::unique_ptr<Component>> components;

	Entity() { AddComponent<TransformComponent>(); }
	Entity(const Entity&) = delete;

	Entity(Entity&& other) : components(std::move(other.components)) 
	{
		for (auto& c :components) {
			c->object = this;
		}
	}

    template<std::derived_from<Component> T, typename... Ts>
    size_t AddComponent(Ts... args)
	{
        auto c = std::make_unique<T>(*this, std::forward<Ts>(args)...);
        components.emplace_back(std::move(c));
        return components.size() - 1;
    }

    template<std::derived_from<Component> T> 
    std::optional<std::reference_wrapper<T>> GetComponent()
	{

        if constexpr(std::is_same_v<T, TransformComponent>) {
            T* cast = dynamic_cast<T*>(components[0].get()); // dynamic cast means if pointer can be converted, convert it 
            if (cast) return *cast;
        }

        for(auto& c : components) {
            T* cast = dynamic_cast<T*>(c.get()); // dynamic cast means if pointer can be converted, convert it 
            if (cast) return *cast;
        }

        return std::nullopt; // nullptr for options 
    }

	template<typename T>
	T* GetComponent() {
		for(auto& component : components) {
			if(auto ptr = dynamic_cast<T*>(component.get())) {
				return ptr;
			}
		}
		return nullptr;
	}

	void tick(float dt) {
		for(auto& component : components) {
			component->tick(dt);
		}
	}
};

struct RenderingComponent : public Component
{
    using Component::Component;
	std::shared_ptr<raylib::Model> modelPtr;
	//RenderingComponent(Entity& e, raylib::Model&& model) is the constructor declaration. It takes two parameters:
	//Entity& e: A reference to the entity object. This is used to access the entity object. 
	//raylib::Model&& model: An rvalue reference to a raylib::Model object. This uses move for transferring ownership of the model without copying it. Efficency nice....
	//Component(e): This is an initializer list that calls the constructor of the base class Component with the Entity reference. 
	//This sets up the RenderingComponent as a specific type of Component.
	RenderingComponent(Entity& e, raylib::Model&& model): Component(e), modelPtr(std::make_shared<raylib::Model>(std::move(model))) { }

	// void setup() override
	// {
	// 	auto ref = object->GetComponent<TransformComponent>();
	// 	if (!ref) return;
	// 	auto& transform = ref->get().rotation; // Access the rotation member instead of the position member
	// 	modelPtr->transform = raylib::Transform(modelPtr->transform).RotateXYZ(transform.x, transform.y, transform.z); // Use transform.x, transform.y, transform.z instead of transform.rotation.x, transform.rotation.y, transform.rotation.z
	// }

	void tick(float dt) override 
	{
		auto transformComponent = object->GetComponent<TransformComponent>();
		if (!transformComponent) return;
		auto& transform = transformComponent->get().position;
		auto backup = modelPtr->transform;
		modelPtr->transform = raylib::Transform(modelPtr->transform).RotateXYZ(transform.x, transform.y, transform.z); // Use transform.x, transform.y, transform.z instead of transform.rotation.x, transform.rotation.y, transform.rotation.z
		modelPtr->Draw({});
		modelPtr->transform = backup;
	}

};


struct PhysicsComponent : Component {
    raylib::Vector3 velocity = {0, 0, 0};
    raylib::Degree object_Heading = 0;
    float object_Speed = 0;
    float speed = 0;
    float acceleration_Rate;
    float turning_Rate;
    float max_Speed;
    PhysicsComponent(Entity& e, float acceleration_Rate, float turning_Rate, float max_Speed) 
        : Component(e), acceleration_Rate(acceleration_Rate), turning_Rate(turning_Rate), max_Speed(max_Speed) { }

void setup() override 
{
	float target_Heading = 0; // Declare the variable 'target_Heading'
	auto transformRef = object->GetComponent<TransformComponent>();
	if (!transformRef) return;
	auto& transform = transformRef->get().rotation;
	//targetHeading = transform.heading;
}

void tick(float dt) override {
float object_Speed = 0; // Declare the variable 'target_Speed'
auto transformRef = object->GetComponent<TransformComponent>();
if (!transformRef) return;
auto& transform = transformRef->get().position;

if(object_Speed > speed)
	speed += acceleration_Rate * dt;
if(object_Speed < speed)
	speed -= acceleration_Rate * dt;
if(object_Heading > transform.y)
	transform.y += turning_Rate * dt;
if(object_Heading < transform.y)
	transform.y -= turning_Rate * dt;

velocity = raylib::Vector3(speed * cos(transform.y), 0, -speed * sin(transform.y));
transformRef->get().position += velocity * dt;}
};

struct InputComponent : public Component
{
    using Component::Component;

    InputComponent(Entity& e) : Component(e) { }

    int entityNumber;
    raylib::BufferedInput inputManager;

    // Input manager setup
    void setup() override {

    int entity = entityNumber;
    auto physicsRef = object->GetComponent<PhysicsComponent>();
    auto& physics = physicsRef->get();

    inputManager["Forward"] = raylib::Action::key(KEY_W).SetPressedCallback([&physics]{
        physics.object_Speed += 20;
    }).move();
    inputManager["Backward"] = raylib::Action::key(KEY_S).SetPressedCallback([&physics]{
        physics.object_Speed -= 20;
    }).move();
    inputManager["Leftward"] = raylib::Action::key(KEY_A).SetPressedCallback([&physics]{
        physics.object_Heading += 60;
    }).move();
    inputManager["Rightward"] = raylib::Action::key(KEY_D).SetPressedCallback([&physics]{
        physics.object_Heading -= 60;
    }).move();
    inputManager["Tabward"] = raylib::Action::key(KEY_TAB).SetPressedCallback([this]{
        entityNumber = (entityNumber + 1) % 10; //
    }).move();
    }
};

struct Component;
struct Entity;
struct TransformComponent;
struct RenderingComponent;
struct PhysicsComponent;
struct InputComponent;

int main() 
{
	// Create window
	const int screenWidth = 800 * 2;
	const int screenHeight = 450 * 2;
	raylib::Window window(screenWidth, screenHeight, "CS381 - Assignment 6");

	std::vector<Entity> entities;
    Entity& plane1 = entities.emplace_back();
    plane1.AddComponent<RenderingComponent>(raylib::Model("meshes/PolyPlane.glb"));
	plane1.AddComponent<PhysicsComponent>(0.1f, 0.1f, 10.0f);
	// Entity& plane2 = entities.emplace_back();
	// plane2.AddComponent<RenderingComponent>(raylib::Model("meshes/PoluPLane.glb"));
	// plane2.AddComponent<TransformComponent>(raylib::Vector3(10, 6, 0), raylib::Quaternion::Identity());
	// plane2.AddComponent<PhysicsComponent>(0.1f, 0.1f, 10.0f);
	// Entity& plane3 = entities.emplace_back();
	// plane3.AddComponent<RenderingComponent>(raylib::Model("meshes/PolyPlane.glb"));
	// plane3.AddComponent<TransformComponent>(raylib::Vector3(20, 10, 0), raylib::Quaternion::Identity());
	// plane3.AddComponent<PhysicsComponent>(0.1f, 0.1f, 10.0f);
	// Entity& plane4 = entities.emplace_back();
	// plane4.AddComponent<RenderingComponent>(raylib::Model("meshes/PolyPlane.glb"));
	// plane4.AddComponent<TransformComponent>(raylib::Vector3(30, 12, 0), raylib::Quaternion::Identity());
	// plane4.AddComponent<PhysicsComponent>(0.1f, 0.1f, 10.0f);
	// Entity& plane5 = entities.emplace_back();
	// plane5.AddComponent<RenderingComponent>(raylib::Model("meshes/PolyPlane.glb"));
	// plane5.AddComponent<TransformComponent>(raylib::Vector3(40, 14, 0), raylib::Quaternion::Identity());
	// plane5.AddComponent<PhysicsComponent>(0.1f, 0.1f, 10.0f);

	Entity& boat1 = entities.emplace_back();
	boat1.AddComponent<RenderingComponent>(raylib::Model("meshes/ddg51.glb"));
	boat1.AddComponent<TransformComponent>(raylib::Vector3(0, 0, 1), raylib::Quaternion::FromAxisAngle(Vector3(0, 0, 1), 90));
	boat1.AddComponent<PhysicsComponent>(0.1f, 0.1f, 10.0f);
	// Entity& boat2 = entities.emplace_back();
	// boat2.AddComponent<RenderingComponent>(raylib::Model("meshes/OilTanker.glb"));
	// boat2.AddComponent<TransformComponent>(raylib::Vector3(8, 0, 0), raylib::Quaternion::Identity());
	// boat2.AddComponent<PhysicsComponent>(0.1f, 0.1f, 10.0f);
	// Entity& boat3 = entities.emplace_back();
	// boat3.AddComponent<RenderingComponent>(raylib::Model("meshesCargoG_HOSBrigadoon.glb"));
	// boat3.AddComponent<TransformComponent>(raylib::Vector3(10, 0, 0), raylib::Quaternion::Identity());
	// boat3.AddComponent<Physic.sComponent>(0.1f, 0.1f, 10.0f);
	// Entity& boat4 = entities.emplace_back();
	// boat4.AddComponent<RenderingComponent>(raylib::Model("meshesCargoG_HOSBrigadoon.glb"));
	// boat4.AddComponent<TransformComponent>(raylib::Vector3(12, 0, 0), raylib::Quaternion::Identity());
	// boat4.AddComponent<PhysicsComponent>(0.1f, 0.1f, 10.0f);
	// Entity& boat5 = entities.emplace_back();
	// boat5.AddComponent<RenderingComponent>(raylib::Model("meshesCargoG_HOSBrigadoon.glb"));
	// boat5.AddComponent<TransformComponent>(raylib::Vector3(14, 0, 0), raylib::Quaternion::Identity());
	// boat5.AddComponent<PhysicsComponent>(0.1f, 0.1f, 10.0f);



	// Create camera
	auto camera = raylib::Camera(
		raylib::Vector3(0, 120, -500), // Position
		raylib::Vector3(0, 0, 300), // Target
		raylib::Vector3::Up(), // Up direction
		45.0f,
		CAMERA_PERSPECTIVE
	);

	// Create skybox
	cs381::SkyBox skybox("textures/skybox.png");

	// Create ground
	auto mesh = raylib::Mesh::Plane(10000, 10000, 50, 50, 25);
	raylib::Model ground = ((raylib::Mesh*)&mesh)->LoadModelFrom();
	raylib::Texture water ("textures/water.jpg");
	water.SetFilter(TEXTURE_FILTER_BILINEAR);
	water.SetWrap(TEXTURE_WRAP_REPEAT);
	ground.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = water;
    std::shared_ptr<raylib::Model> airplanePtr = std::make_shared<raylib::Model>(LoadModel("meshes/PolyPlane.glb"));
    std::shared_ptr<raylib::Model> shipPtr = std::make_shared<raylib::Model>(LoadModel("meshes/ddg51.glb"));

    std::string planePath = "meshes/PolyPlane.glb";
    std::string shipPath = "meshes/ddg51.glb";

	// Main loop
	bool keepRunning = true;
	while(!window.ShouldClose() && keepRunning) {
		// Updates
		// Process input for the selected plane

		// Rendering
		window.BeginDrawing();
		{
			// Clear screen
			window.ClearBackground(BLACK);

			camera.BeginMode();
			{
				// Render skybox and ground
				skybox.Draw();
				ground.Draw({});
				 // Drawing all of the airplanes
                for(Entity& e: entities)
                e.tick(window.GetFrameTime());
			}
			camera.EndMode();

			// Measure our FPS
			DrawFPS(10, 10);
		}
		window.EndDrawing();
	}

	return 0;
}

