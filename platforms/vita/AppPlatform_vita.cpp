#include <sys/stat.h>

#include "AppPlatform_vita.hpp"

AppPlatform_vita::AppPlatform_vita(std::string storageDir, SDL_Window* window)
	: AppPlatform_sdl2(storageDir, window)
{
}

bool AppPlatform_vita::hasFileSystemAccess()
{
	return true;
}

std::string AppPlatform_vita::getAssetPath(const std::string& path) const
{
	// Assets are packed inside the VPK, relative paths are not usable on the Vita
	return "app0:/assets/" + path;
}

void AppPlatform_vita::recenterMouse()
{
	// No mouse to recenter on a touchscreen
}

void AppPlatform_vita::_ensureDirectoryExists(const char* path)
{
	mkdir(path, 0777);
}

void AppPlatform_vita::_updateWindowIcon()
{
	// No window icons on the Vita
}
