#pragma once

#include <string>

#include "../sdl/sdl2/base/AppPlatform_sdl2.hpp"

class AppPlatform_vita : public AppPlatform_sdl2
{
public:
	AppPlatform_vita(std::string storageDir, SDL_Window* window);

	bool hasFileSystemAccess() override;

	std::string getAssetPath(const std::string& path) const override;

	void recenterMouse() override;

protected:
	void _ensureDirectoryExists(const char* path) override;
	void _updateWindowIcon() override;
};
