//
//  Wasm3 - high performance WebAssembly interpreter written in C.
//
//  Copyright © 2019 Steven Massey, Volodymyr Shymanskyy.
//  All rights reserved.
//

#include "esp_system.h"

#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include "wasm3.h"
#include "m3_env.h"

#include "m3_api_esp_wasi.h"
#include "m3_http.h"
// #include "wasi_test.wasm.h"
// #include "test_wasm.h"
#include "http_wasm.h"

//#define CONFIG_EXAMPLE_CONNECT_WIFI 1
//#define EXAMPLE_NETIF_DESC_STA "example_netif_sta"
//#include "example_common_private.h"

#include "wifi.h"

#define FATAL(msg, ...) { printf("Fatal: " msg "\n", ##__VA_ARGS__); return; }

static void run_wasm(void)
{
    M3Result result = m3Err_none;

    uint8_t* wasm = (uint8_t*)http_wasm;
    uint32_t fsize = http_wasm_len;

    printf("Loading WebAssembly...\n");
    IM3Environment env = m3_NewEnvironment ();
    if (!env) FATAL("m3_NewEnvironment failed");

    IM3Runtime runtime = m3_NewRuntime (env, 8*1024, NULL);
    if (!runtime) FATAL("m3_NewRuntime failed");

    IM3Module module;
    result = m3_ParseModule (env, &module, wasm, fsize);
    if (result) FATAL("m3_ParseModule: %s", result);

    result = m3_LoadModule (runtime, module);
    if (result) FATAL("m3_LoadModule: %s", result);

    result = m3_LinkEspWASI (runtime->modules);
    if (result) FATAL("m3_LinkEspWASI: %s", result);
    
    result = m3_LinkHttp (runtime->modules);
    if (result) FATAL("m3_LinkHttp: %s", result);

    IM3Function f;
    result = m3_FindFunction (&f, runtime, "_start");
    if (result) FATAL("m3_FindFunction: %s", result);

    printf("Running...\n");

    const char* i_argv[2] = { "test.wasm", NULL };

    m3_wasi_context_t* wasi_ctx = m3_GetWasiContext();
    wasi_ctx->argc = 1;
    wasi_ctx->argv = i_argv;

    result = m3_CallV (f);

    if (result) FATAL("m3_Call: %s", result);
}

extern "C" void app_main(void)
{
    printf("\nWasm3 v" M3_VERSION " on " CONFIG_IDF_TARGET ", build " __DATE__ " " __TIME__ "\n");

    // example_wifi_connect();

    WF wifi;
    wifi.sta_connect("ubnt", "2065886782", 1);

    clock_t start = clock();
    run_wasm();
    clock_t end = clock();

    printf("Elapsed: %ld ms\n", (end - start)*1000 / CLOCKS_PER_SEC);

    sleep(3000);
    printf("Restarting...\n\n\n");
    esp_restart();
}
