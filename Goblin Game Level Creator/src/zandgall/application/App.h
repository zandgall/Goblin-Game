#pragma once
class App {
public:
	static App* instance;
	App();

	~App() {

	}

	void tick(double delta);
	void render();
};

