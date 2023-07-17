#include "Renderer.h"

int main(int argc, char** argv)
{
	std::unique_ptr<Renderer> renderer = std::make_unique<Renderer>();
	renderer->Run();
	return 0;
}

//chirag 2023