#include "MumblePlugin_v_1_0_x.h"
#include "logfile.c"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FACTORIO_EXE "factorio.exe"

struct MumbleAPI_v_1_0_x mumbleAPI;
mumble_plugin_id_t ownID;

bool factorio_mod_notified = false; // notify once to install Factorio mod

mumble_error_t mumble_init(mumble_plugin_id_t pluginID)
{
	ownID = pluginID;

	srand(81731);

	if (mumbleAPI.log(ownID, "Hello Mumble") != MUMBLE_STATUS_OK)
	{
		// Logging failed -> usually you'd probably want to log things like this in your plugin's
		// logging system (if there is any)
	}

	return MUMBLE_STATUS_OK;
}

void mumble_shutdown()
{
	if (mumbleAPI.log(ownID, "Goodbye Mumble") != MUMBLE_STATUS_OK)
	{
		// Logging failed -> usually you'd probably want to log things like this in your plugin's
		// logging system (if there is any)
	}
}

struct MumbleStringWrapper mumble_getName()
{
	static const char *name = "Factorio";

	struct MumbleStringWrapper wrapper;
	wrapper.data = name;
	wrapper.size = strlen(name);
	wrapper.needsReleasing = false;

	return wrapper;
}

mumble_version_t mumble_getAPIVersion()
{
	// This constant will always hold the API version  that fits the included header files
	return MUMBLE_PLUGIN_API_VERSION;
}

void mumble_registerAPIFunctions(void *apiStruct)
{
	// Provided mumble_getAPIVersion returns MUMBLE_PLUGIN_API_VERSION, this cast will make sure
	// that the passed pointer will be cast to the proper type
	mumbleAPI = MUMBLE_API_CAST(apiStruct);
}

void mumble_releaseResource(const void *pointer)
{
	// As we never pass a resource to Mumble that needs releasing, this function should never
	// get called
	printf("Called mumble_releaseResource but expected that this never gets called -> Aborting");
	abort();
}

// Below functions are not strictly necessary but every halfway serious plugin should implement them nonetheless

mumble_version_t mumble_getVersion()
{
	char *VERSION = "0.2.0";

	mumble_version_t version;
	char *v = malloc(strlen(VERSION) + 1);
	strcpy(v, VERSION); // cannot use strtok on const char*
	version.major = atoi(strtok(v, "."));
	version.minor = atoi(strtok(NULL, "."));
	version.patch = atoi(strtok(NULL, "."));

	return version;
}

struct MumbleStringWrapper mumble_getAuthor()
{
	static const char *author = "alifeee";

	struct MumbleStringWrapper wrapper;
	wrapper.data = author;
	wrapper.size = strlen(author);
	wrapper.needsReleasing = false;

	return wrapper;
}

struct MumbleStringWrapper mumble_getDescription()
{
	static const char *description = "Factorio plugin for Mumble's Positional Audio API. Requires Factorio positional audio mod.";

	struct MumbleStringWrapper wrapper;
	wrapper.data = description;
	wrapper.size = strlen(description);
	wrapper.needsReleasing = false;

	return wrapper;
}

// positional audio functions

uint32_t mumble_getFeatures()
{
	return MUMBLE_FEATURE_POSITIONAL;
}

uint8_t mumble_initPositionalData(const char *const *programNames, const uint64_t *programPIDs, size_t programCount)
{
	// Check if the supported game is in the list of programs and if yes
	// check if the position can be obtained from the program

	// programNames is a list of, e.g., ["Notion.exe", "System", "firefox.exe", "factorio.exe"]

	// loop through programs, if FACTORIO_EXE is found, return MUMBLE_PDEC_OK
	bool found = false;
	for (size_t i = 0; i < programCount; i++)
	{
		if (strcmp(programNames[i], FACTORIO_EXE) == 0)
		{
			found = true;
			break;
		}
	}

	if (!found)
	{
		// If the game is not running, return MUMBLE_PDEC_ERROR_TEMP
		return MUMBLE_PDEC_ERROR_TEMP;
	}

	// If the game is running, check if the positional audio mod is installed (if the log file exists)
	// and if it is, return MUMBLE_PDEC_OK
	if (!is_factorio_logfile_there())
	{
		// If the game is running but the positional audio mod is not installed, notify the user
		// to install the mod
		if (!factorio_mod_notified)
		{
			mumbleAPI.log(ownID, "Factorio positional audio mod not installed. Please install it to use positional audio.");
			factorio_mod_notified = true;
		}
		return MUMBLE_PDEC_ERROR_TEMP;
	}

	if (!is_factorio_logfile_recent(2)) // 2 seconds
	{
		return MUMBLE_PDEC_ERROR_TEMP;
	}

	return MUMBLE_PDEC_OK;

	// Other potential return values are:
	// MUMBLE_PDEC_ERROR_TEMP -> The plugin can temporarily not deliver positional data
	// MUMBLE_PDEC_PERM -> Permanenet error. The plugin will never be able to deliver positional data
}

bool mumble_fetchPositionalData(float *avatarPos, float *avatarDir, float *avatarAxis, float *cameraPos, float *cameraDir,
								float *cameraAxis, const char **context, const char **identity)
{
	// fetch positional data and store it in the respective variables. All fields that can't be filled properly
	// have to be set to 0 or the empty String ""

	// if log file does not exist, OH NO!
	if (!is_factorio_logfile_there())
	{
		return false;
	}

	// if log file is old (last modified > N seconds ago), data is bad
	if (!is_factorio_logfile_recent(2)) // 2 seconds
	{
		return false;
	}

	// if log file exists, parse it
	float x, y, z;
	int player, surface;
	char *server;
	size_t server_len;

	parse_factorio_logfile(&x, &y, &z, &player, &surface, &server, &server_len);

	avatarPos[0] = x;
	avatarPos[1] = y;
	avatarPos[2] = z;

	avatarDir[0] = 0.0f;
	avatarDir[1] = 0.0f;
	avatarDir[2] = 0.0f;

	avatarAxis[0] = 0.0f;
	avatarAxis[1] = 0.0f;
	avatarAxis[2] = 0.0f;

	cameraPos[0] = 0.0f;
	cameraPos[1] = 0.0f;
	cameraPos[2] = 0.0f;

	cameraDir[0] = 0.0f;
	cameraDir[1] = 0.0f;
	cameraDir[2] = 0.0f;

	cameraAxis[0] = 0.0f;
	cameraAxis[1] = 0.0f;
	cameraAxis[2] = 0.0f;

	// context: combine server + surface
	char *context_str = malloc(server_len + 1 + 1 + 1);
	strcpy(context_str, server);
	strcat(context_str, "/");
	char surface_str[12];
	sprintf(surface_str, "%d", surface);
	strcat(context_str, surface_str);
	*context = context_str;

	// identity: player
	char player_str[12];
	sprintf(player_str, "%d", player);
	*identity = player_str;

	// If positional data could be fetched successfully
	return true;
	// otherwise return false
}

void mumble_shutdownPositionalData()
{
	// Unlink the connection to the supported game
	// Perform potential clean-up code
}