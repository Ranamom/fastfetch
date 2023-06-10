#include "fastfetch.h"
#include "common/printing.h"
#include "detection/opengl/opengl.h"
#include "modules/opengl/opengl.h"

#define FF_OPENGL_NUM_FORMAT_ARGS 4

void ffPrintOpenGL(FFinstance* instance, FFOpenGLOptions* options)
{
    FFOpenGLResult result;
    ffStrbufInit(&result.version);
    ffStrbufInit(&result.renderer);
    ffStrbufInit(&result.vendor);
    ffStrbufInit(&result.slv);

    const char* error = ffDetectOpenGL(instance, &result);
    if(error)
    {
        ffPrintError(instance, FF_OPENGL_MODULE_NAME, 0, &options->moduleArgs, "%s", error);
        return;
    }

    if(options->moduleArgs.outputFormat.length == 0)
    {
        ffPrintLogoAndKey(instance, FF_OPENGL_MODULE_NAME, 0, &options->moduleArgs.key);
        puts(result.version.chars);
    }
    else
    {
        ffPrintFormat(instance, FF_OPENGL_MODULE_NAME, 0, &options->moduleArgs, FF_OPENGL_NUM_FORMAT_ARGS, (FFformatarg[]) {
            {FF_FORMAT_ARG_TYPE_STRBUF, &result.version},
            {FF_FORMAT_ARG_TYPE_STRBUF, &result.renderer},
            {FF_FORMAT_ARG_TYPE_STRBUF, &result.vendor},
            {FF_FORMAT_ARG_TYPE_STRBUF, &result.slv}
        });
    }

    ffStrbufDestroy(&result.version);
    ffStrbufDestroy(&result.renderer);
    ffStrbufDestroy(&result.vendor);
    ffStrbufDestroy(&result.slv);
}

void ffInitOpenGLOptions(FFOpenGLOptions* options)
{
    options->moduleName = FF_OPENGL_MODULE_NAME;
    ffOptionInitModuleArg(&options->moduleArgs);

    #if defined(__linux__) || defined(__FreeBSD__)
    options->type = FF_OPENGL_TYPE_AUTO;
    #endif
}

bool ffParseOpenGLCommandOptions(FFOpenGLOptions* options, const char* key, const char* value)
{
    const char* subKey = ffOptionTestPrefix(key, FF_OPENGL_MODULE_NAME);
    if (!subKey) return false;
    if (ffOptionParseModuleArgs(key, subKey, value, &options->moduleArgs))
        return true;

    #if defined(__linux__) || defined(__FreeBSD__)
    options->type = (FFOpenGLType) ffOptionParseEnum(key, value, (FFKeyValuePair[]) {
        { "auto", FF_OPENGL_TYPE_AUTO },
        { "egl", FF_OPENGL_TYPE_EGL },
        { "glx", FF_OPENGL_TYPE_GLX },
        { "osmesa", FF_OPENGL_TYPE_OSMESA },
        {}
    });
    #endif

    return false;
}

void ffDestroyOpenGLOptions(FFOpenGLOptions* options)
{
    ffOptionDestroyModuleArg(&options->moduleArgs);
}

#ifdef FF_HAVE_JSONC
void ffParseOpenGLJsonObject(FFinstance* instance, json_object* module)
{
    FFOpenGLOptions __attribute__((__cleanup__(ffDestroyOpenGLOptions))) options;
    ffInitOpenGLOptions(&options);

    if (module)
    {
        json_object_object_foreach(module, key, val)
        {
            if (ffJsonConfigParseModuleArgs(key, val, &options.moduleArgs))
                continue;

            #if defined(__linux__) || defined(__FreeBSD__)
            if (strcasecmp(key, "type") == 0)
            {
                int value;
                const char* error = ffJsonConfigParseEnum(val, &value, (FFKeyValuePair[]) {
                    { "auto", FF_OPENGL_TYPE_AUTO },
                    { "egl", FF_OPENGL_TYPE_EGL },
                    { "glx", FF_OPENGL_TYPE_GLX },
                    { "osmesa", FF_OPENGL_TYPE_OSMESA },
                    {},
                });
                if (error)
                    ffPrintError(instance, FF_OPENGL_MODULE_NAME, 0, &options.moduleArgs, "Invalid %s value: %s", key, error);
                else
                    options.type = (FFOpenGLType) value;
                continue;
            }
            #endif

            ffPrintError(instance, FF_OPENGL_MODULE_NAME, 0, &options.moduleArgs, "Unknown JSON key %s", key);
        }
    }

    ffPrintOpenGL(instance, &options);
}
#endif
