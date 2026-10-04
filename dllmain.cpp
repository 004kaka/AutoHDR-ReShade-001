/* ============================================================================
 * Project: AutoHDR Minimalist scRGB Bridge for Judgment (Dragon Engine Edition)
 * Base   : EndlesslyFlowering/AutoHDR-ReShade (mine branch)
 * Target : Judgment (Steam D3D11) + Shin Ryu Mod Manager (version.dll)
 * 
 * [Bilingual Annotation & Preservation Policy / 이중 언어 주석 및 무삭제 보존 원칙]
 * 1. Zero-Deletion Policy: Not a single line of original code is removed.
 *    All unused or crash-inducing legacy codes are 100% preserved via block comments.
 * 2. Anti-Inversion Bilingual Comments: English and rigorously verified Korean
 *    are paired to prevent semantic inversion (positive/negative, max/min).
 * ----------------------------------------------------------------------------
 * 1. 무삭제 원칙: 원본 소스코드의 단 한 줄도 임의로 삭제하지 않았습니다.
 *    현재 사용하지 않거나 충돌을 유발하는 레거시 코드는 전량 블록 주석으로 보존합니다.
 * 2. 의미 반전 방지 이중 주석: 기술적 의미 왜곡(긍정/부정, 최대/최소)을 원천 차단하기 위해
 *    영문 설명과 정밀 대조 번역된 한국어 설명을 나란히 병기합니다.
 * ============================================================================ */

#include "pch.h"

#define ImTextureID unsigned long long int

#include <stdio.h>
#include <imgui.h>
#include <reshade.hpp>
#include <dxgi1_6.h>
#include <d3d11.h>
#include <d3d12.h>
#include <mutex>
#include <sstream>
#include <unordered_set>
#include <cassert>
#include <atlbase.h>

/* ============================================================================
 * [DISABLED / PRESERVED ORIGINAL CODE / 비활성화 및 원본 보존 구역]
 * ----------------------------------------------------------------------------
 * [English Description]:
 * - Original Code       : #include "debug.h"
 * - Reason for Disabling: "debug.h" is an external repository dependency. Disabling it
 *                         enables 100% standalone single-file building on GitHub Actions CI.
 * - Alternative Provided: Lightweight inline format/color space string helpers are
 *                         implemented directly below to provide human-readable logs.
 * - Revival Potential   : Preserved strictly if external debug headers are reintroduced.
 * ----------------------------------------------------------------------------
 * [한국어 상세 설명 (정밀 대조 번역)]:
 * - 원본 코드: #include "debug.h"
 * - 비활성화 이유: "debug.h"는 외부 리포지토리 파일 의존성입니다. 단일 파일만으로 깃허브
 *                 액션(CI)에서 에러 없이 100% 독립 빌드되도록 비활성화합니다.
 * - 대체 구현: 외부 파일 없이도 로그에 사람이 읽을 수 있는 포맷 이름을 남기기 위해
 *             가벼운 자체 인라인 문자열 헬퍼 함수를 아래에 직접 구현했습니다.
 * - 복구 가능성: 추후 외부 디버그 헤더를 다시 연동할 경우를 위해 원본을 보존합니다.
 * ============================================================================ */
/*
#include "debug.h"
*/

//#define __WINRT__

// ============================================================================
// [ACTIVE MINIMAL PIPELINE / 핵심 동작 구역]
// ----------------------------------------------------------------------------
// [English Description]:
// - Functionality: Built-in inline helper functions for DXGI format & color space names.
// - Purpose      : Eliminates the missing "debug.h" dependency while keeping logs readable.
// ----------------------------------------------------------------------------
// [한국어 상세 설명 (정밀 대조 번역)]:
// - 동작 기능: DXGI 포맷 및 색 공간 번호를 사람이 읽을 수 있는 텍스트로 변환하는 인라인 헬퍼.
// - 목적: "debug.h" 의존성을 없애면서도 로그 파일에 단순 숫자가 아닌 명확한 포맷 이름을 기록합니다.
// ============================================================================
inline static const char* GetDxgiFormatName(DXGI_FORMAT format)
{
    switch (format)
    {
    case DXGI_FORMAT_R16G16B16A16_FLOAT: return "DXGI_FORMAT_R16G16B16A16_FLOAT";
    case DXGI_FORMAT_R10G10B10A2_UNORM:  return "DXGI_FORMAT_R10G10B10A2_UNORM";
    case DXGI_FORMAT_R8G8B8A8_UNORM:      return "DXGI_FORMAT_R8G8B8A8_UNORM";
    case DXGI_FORMAT_B8G8R8A8_UNORM:      return "DXGI_FORMAT_B8G8R8A8_UNORM";
    default:                              return "DXGI_FORMAT_OTHER";
    }
}

inline static const char* GetDxgiColorSpaceName(DXGI_COLOR_SPACE_TYPE color_space)
{
    switch (color_space)
    {
    case DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709:   return "RGB_FULL_G10_NONE_P709 (scRGB Linear)";
    case DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020: return "RGB_FULL_G2084_NONE_P2020 (HDR10 PQ)";
    case DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709:   return "RGB_FULL_G22_NONE_P709 (sRGB)";
    default:                                         return "COLOR_SPACE_OTHER";
    }
}

// ============================================================================
// [ACTIVE MINIMAL PIPELINE / 핵심 동작 구역]
// ----------------------------------------------------------------------------
// [English Description]:
// - Functionality: Standalone persistent file logger writing to "scRGB_addon_log.txt".
// - Purpose      : GitHub Actions creates Release builds where standard debug output is lost.
//                  This class guarantees real-time file logging in both Debug and Release.
// ----------------------------------------------------------------------------
// [한국어 상세 설명 (정밀 대조 번역)]:
// - 동작 기능: 게임 폴더의 "scRGB_addon_log.txt" 파일에 실시간 기록하는 독립형 상시 로거.
// - 목적: 깃허브 액션의 Release 빌드 환경에서도 로그가 증발하지 않고 디버깅 정보를 남기도록 보증합니다.
// ============================================================================
class SafeLogManager
{
public:
    SafeLogManager()
    {
        errno_t error = _wfopen_s(&log_file, L"scRGB_addon_log.txt", L"w");
        (void)error;
    }

    ~SafeLogManager()
    {
        if (log_file != nullptr)
        {
            fflush(log_file);
            fclose(log_file);
            log_file = nullptr;
        }
    }

    inline void Write(const wchar_t* format, ...)
    {
        va_list args;
        va_start(args, format);
        OutputDebugStringW(format);
        if (log_file != nullptr)
        {
            vfwprintf(log_file, format, args);
            fflush(log_file);
        }
        va_end(args);
    }

private:
    FILE* log_file = nullptr;
};

static SafeLogManager g_custom_logger;

inline void LogToFile(const wchar_t* format, ...)
{
    va_list args;
    va_start(args, format);
    // Write via SafeLogManager
    wchar_t buffer[1024];
    vswprintf_s(buffer, 1024, format, args);
    g_custom_logger.Write(L"%s", buffer);
    va_end(args);
}

std::unordered_set<uint64_t> g_back_buffers;
std::mutex g_mutex;

/* ============================================================================
 * [ACTIVE & PRESERVED GLOBAL VARIABLES / 전역 변수 설정 구역]
 * ----------------------------------------------------------------------------
 * [English Description]:
 * - g_hdr_enable is explicitly forced to TRUE (instead of original default FALSE).
 * - Reason: If default is false and ReShade runtime fails to read config at startup,
 *           the code enters the SDR demotion loop and crashes via ResizeBuffers.
 * ----------------------------------------------------------------------------
 * [한국어 상세 설명 (정밀 대조 번역)]:
 * - g_hdr_enable 변수를 기본값 FALSE에서 TRUE로 명시적으로 변경하여 고정합니다.
 * - 이유: 기본값이 false인 상태에서 ini 설정을 제때 읽지 못하면 SDR 강등 루프로 진입하여
 *         ResizeBuffers를 부르다가 게임이 튕기는 문제를 방지하기 위함입니다.
 * ============================================================================ */
bool                          g_hdr_enable       = true; // Forced TRUE for reliable pipeline / 상시 활성화 고정
bool                          g_use_hdr10        = false;
bool                          g_hdr_support      = false;
bool                          g_first_csp_change = true;
DXGI_COLOR_SPACE_TYPE         g_colour_space     = DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;
DXGI_FORMAT                   g_original_format  = DXGI_FORMAT_R10G10B10A2_UNORM;
bool                          g_is_supported_api = false;
bool                          g_is_vulkan_api    = false;

reshade::api::device*         g_device           = nullptr;
reshade::api::effect_runtime* g_runtime          = nullptr;

inline static int dxgi_compute_intersection_area(
    int ax1, int ay1, int ax2, int ay2,
    int bx1, int by1, int bx2, int by2)
{
    return  max(0, min(ax2, bx2) -
            max(ax1, bx1))
            * max(0, min(ay2, by2) - max(ay1, by1));
}

/* ============================================================================
 * [DISABLED / PRESERVED ORIGINAL CODE / 비활성화 및 원본 보존 구역]
 * ----------------------------------------------------------------------------
 * [English Description]:
 * - Original Function: dxgi_check_display_hdr_support()
 * - Reason for Disabling: Strictly checks for HDR10 (DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020).
 *                         In scRGB environments, this function returns FALSE and aborts
 *                         the scRGB initialization. scRGB relies on native CheckColorSpaceSupport.
 * - Revival Potential   : 100% preserved line-by-line for future HDR10 fallback references.
 * ----------------------------------------------------------------------------
 * [한국어 상세 설명 (정밀 대조 번역)]:
 * - 원본 함수: dxgi_check_display_hdr_support()
 * - 비활성화 이유: 이 함수는 오직 HDR10 (G2084) 신호만을 감지합니다. scRGB 환경에서는 
 *                 불필요하게 FALSE를 반환하여 색 공간 초기화 전체를 차단하므로 비활성화합니다.
 *                 scRGB 지원 검사는 DXGI 네이티브 CheckColorSpaceSupport를 직접 사용합니다.
 * - 복구 가능성: 추후 HDR10 폴백 기능 추가 시 참조를 위해 단 한 줄도 지우지 않고 보존합니다.
 * ============================================================================ */
/*
#ifdef __WINRT__
bool dxgi_check_display_hdr_support(IDXGIFactory2* factory, HWND hwnd)
#else
bool dxgi_check_display_hdr_support(IDXGIFactory1* factory, HWND hwnd)
#endif
{
    IDXGIOutput6* output6 = NULL;
    IDXGIOutput* best_output = NULL;
    IDXGIOutput* current_output = NULL;
    IDXGIAdapter* dxgi_adapter = NULL;
    UINT i = 0;
    bool supported = false;
    float best_intersect_area = -1;

#ifdef __WINRT__
    if (!factory->IsCurrent())
    {
        if (FAILED(CreateDXGIFactory2(0, __uuidof(IDXGIFactory2), (void**)&factory)))
        {
            reshade::log::message(reshade::log::level::error, "[DXGI]: Failed to create DXGI factory");
            return false;
        }
    }

    if (FAILED(factory->EnumAdapters(0, &dxgi_adapter)))
    {
        reshade::log::message(reshade::log::level::error, "[DXGI]: Failed to enumerate adapters");
        return false;
    }
#else
    if (!factory->IsCurrent())
    {
        if (FAILED(CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&factory)))
        {
            reshade::log::message(reshade::log::level::error, "[DXGI]: Failed to create DXGI factory");
            return false;
        }
    }

    if (FAILED(factory->EnumAdapters(0, &dxgi_adapter)))
    {
        reshade::log::message(reshade::log::level::error, "[DXGI]: Failed to enumerate adapters");
        return false;
    }
#endif

    while (dxgi_adapter->EnumOutputs(i, &current_output)
        != DXGI_ERROR_NOT_FOUND)
    {
        RECT r, rect;
        DXGI_OUTPUT_DESC desc;
        int intersect_area;
        int bx1, by1, bx2, by2;
        int ax1 = 0;
        int ay1 = 0;
        int ax2 = 0;
        int ay2 = 0;

        if (GetWindowRect(hwnd, &rect))
        {
            ax1 = rect.left;
            ay1 = rect.top;
            ax2 = rect.right;
            ay2 = rect.bottom;
        }

        if (FAILED(current_output->GetDesc(&desc)))
        {
            reshade::log::message(reshade::log::level::error, "[DXGI]: Failed to get DXGI output description");
            goto error;
        }

        r = desc.DesktopCoordinates;
        bx1 = r.left;
        by1 = r.top;
        bx2 = r.right;
        by2 = r.bottom;

        intersect_area = dxgi_compute_intersection_area(
            ax1, ay1, ax2, ay2, bx1, by1, bx2, by2);

        if (intersect_area > best_intersect_area)
        {
            best_output = current_output;
            best_output->AddRef();
            best_intersect_area = (float)intersect_area;
        }

        i++;
    }

    if (SUCCEEDED(best_output->QueryInterface(__uuidof(IDXGIOutput6), (void**)&output6)))
    {
        DXGI_OUTPUT_DESC1 desc1;
        if (SUCCEEDED(output6->GetDesc1(&desc1)))
        {
            supported = (desc1.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020);

            if (supported)
            {
                reshade::log::message(reshade::log::level::info, "[DXGI]: DXGI Output supports: DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020");
            }

            g_hdr_support = supported;
        }
        else
        {
            reshade::log::message(reshade::log::level::error, "[DXGI]: Failed to get DXGI Output 6 description");
        }
        output6->Release();
    }
    else
    {
        reshade::log::message(reshade::log::level::error, "[DXGI]: Failed to get DXGI Output 6 from best output");
    }

error:
    if(best_output) best_output->Release();
    if(current_output) current_output->Release();
    if(dxgi_adapter) dxgi_adapter->Release();

    return supported;
}
*/

// ============================================================================
// [ACTIVE MINIMAL PIPELINE / 핵심 동작 구역]
// ----------------------------------------------------------------------------
// [English Description]:
// - Functionality: set_reshade_colour_space()
// - Purpose      : Notifies the ReShade runtime that the back buffer is in 
//                  extended_srgb_linear (scRGB) so that ReShade shaders (.fx)
//                  can correctly process high-dynamic-range linear inputs.
// ----------------------------------------------------------------------------
// [한국어 상세 설명 (정밀 대조 번역)]:
// - 동작 기능: set_reshade_colour_space()
// - 목적: ReShade 런타임에 현재 백버퍼가 extended_srgb_linear (scRGB 선형 공간)임을
//         알려주어, ReShade 셰이더들(.fx)이 넓은 밝기 범위를 올바르게 인식하고 렌더링하게 합니다.
// ============================================================================
void set_reshade_colour_space()
{
    if (g_runtime != nullptr)
    {
        reshade::api::color_space reshade_colour_space;

        switch(g_colour_space)
        {
            case DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709:
            {
                reshade_colour_space = reshade::api::color_space::extended_srgb_linear;
            }
            break;
            case DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020:
            {
                reshade_colour_space = reshade::api::color_space::hdr10_st2084;
            }
            break;
            default:
            {
                reshade_colour_space = reshade::api::color_space::srgb_nonlinear;
            }
            break;
        }

        std::stringstream log_str;
        log_str << "[ReShade]: ReShade colour space "
                << GetDxgiColorSpaceName(g_colour_space)
                << " set";

        reshade::log::message(reshade::log::level::info, log_str.str().c_str());
        LogToFile(L"[ReShade]: Color space set to %hs\n", GetDxgiColorSpaceName(g_colour_space));

        g_runtime->set_color_space(reshade_colour_space);
    }
}

// ============================================================================
// [ACTIVE MINIMAL PIPELINE / 핵심 동작 구역]
// ----------------------------------------------------------------------------
// [English Description]:
// - Functionality: dxgi_swapchain_color_space() with Graceful Fallback.
// - Purpose      : Queries scRGB PRESENT support and calls SetColorSpace1().
//                  Crucially, if SetColorSpace1 fails, it logs an error but DOES NOT
//                  crash or abort the game, ensuring the process remains fully playable.
// ----------------------------------------------------------------------------
// [한국어 상세 설명 (정밀 대조 번역)]:
// - 동작 기능: 안전 폴백(Graceful Fallback)이 포함된 dxgi_swapchain_color_space()
// - 목적: scRGB PRESENT 지원 여부를 확인하고 SetColorSpace1()을 호출합니다.
//         가장 중요한 점은, 만에 하나 실패하더라도 게임 프로세스를 강제 종료(abort)하지 않고
//         에러 로그만 기록한 채 정상 통과시켜 게임 플레이가 지속되도록 안전망을 보장합니다.
// ============================================================================
void dxgi_swapchain_color_space(
    IDXGISwapChain3*      swapchain,
    DXGI_COLOR_SPACE_TYPE target_colour_space)
{
    if (swapchain == nullptr)
    {
        LogToFile(L"[DXGI]: Null swapchain passed to dxgi_swapchain_color_space\n");
        return;
    }

    UINT color_space_support = 0;

    HRESULT hr_check = swapchain->CheckColorSpaceSupport(target_colour_space, &color_space_support);
    if (FAILED(hr_check))
    {
        reshade::log::message(reshade::log::level::error, "[DXGI]: Failed to check DXGI swapchain colour space support");
        LogToFile(L"[DXGI]: CheckColorSpaceSupport failed: HRESULT 0x%08X\n", hr_check);
        return; // Graceful return without crash / 강제 종료 없이 안전 반환
    }

    if ((color_space_support & DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT) == DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT)
    {
        HRESULT hr_set = swapchain->SetColorSpace1(target_colour_space);
        if (FAILED(hr_set))
        {
            reshade::log::message(reshade::log::level::error, "[DXGI]: Failed to set DXGI swapchain colour space");
            LogToFile(L"[DXGI]: SetColorSpace1 failed: HRESULT 0x%08X\n", hr_set);
            return; // Graceful return without crash / 강제 종료 없이 안전 반환
        }

        std::stringstream log_str;
        log_str << "[DXGI]: DXGI swapchain colour space "
                << GetDxgiColorSpaceName(target_colour_space)
                << " set";

        reshade::log::message(reshade::log::level::info, log_str.str().c_str());
        LogToFile(L"[DXGI]: Successfully applied %hs\n", GetDxgiColorSpaceName(target_colour_space));

        g_colour_space = target_colour_space;

        set_reshade_colour_space();
    }
    else
    {
        std::stringstream log_str;
        log_str << "[DXGI]: DXGI swapchain colour space "
                << GetDxgiColorSpaceName(target_colour_space)
                << " ("
                << color_space_support
                << ") not supported for presentation";

        reshade::log::message(reshade::log::level::error, log_str.str().c_str());
        LogToFile(L"[DXGI]: scRGB PRESENT flag not supported (flags: 0x%08X)\n", color_space_support);
    }
}
static void on_init_device(reshade::api::device* device)
{
    //device->create_private_data<state_tracking_context>();

    g_device = device;

    const reshade::api::device_api device_type = device->get_api();

    if (device_type == reshade::api::device_api::d3d10
     || device_type == reshade::api::device_api::d3d11
     || device_type == reshade::api::device_api::d3d12
     || device_type == reshade::api::device_api::vulkan)
    {
        g_is_supported_api = true;
    }
    else
    {
        g_is_supported_api = false;
    }

    if (device_type == reshade::api::device_api::vulkan)
    {
        g_is_vulkan_api = true;
    }
    else
    {
        g_is_vulkan_api = false;
    }

    /* ============================================================================
     * [DISABLED / PRESERVED ORIGINAL CODE / 비활성화 및 원본 보존 구역]
     * ----------------------------------------------------------------------------
     * [English Description]:
     * - Original Code       : Calling reshade::get_config_value() inside on_init_device.
     * - Reason for Disabling: At the time on_init_device fires, g_runtime is nullptr!
     *                         Passing nullptr fails to read the configuration, leaving
     *                         g_hdr_enable as false and triggering an SDR demotion loop.
     * - Revival Potential   : Preserved for reference if runtime-independent config is used.
     * ----------------------------------------------------------------------------
     * [한국어 상세 설명 (정밀 대조 번역)]:
     * - 원본 코드: on_init_device 내부에서 reshade::get_config_value()를 호출하여 설정 읽기.
     * - 비활성화 이유: on_init_device 시점에는 g_runtime이 아직 nullptr 상태입니다!
     *                 nullptr을 전달하면 설정을 읽지 못해 g_hdr_enable이 false로 남아
     *                 SDR 강등 루프를 유발하므로 비활성화하고, 전역 변수에서 상시 활성화합니다.
     * - 복구 가능성: 추후 런타임 비의존적 설정 로딩 구현 시 참조를 위해 보존합니다.
     * ============================================================================ */
    /*
    if (g_is_supported_api)
    {
        reshade::get_config_value(g_runtime, "HDR", "EnableHDR", g_hdr_enable);
        reshade::get_config_value(g_runtime, "HDR", "UseHDR10",  g_use_hdr10);
    }
    */

    LogToFile(L"[Device]: Device initialized (API: %u, Supported: %s)\n",
        static_cast<unsigned int>(device_type),
        g_is_supported_api ? L"YES" : L"NO");
}

static void on_destroy_device(reshade::api::device* device)
{
    g_device = nullptr;

    g_is_supported_api = false;
    g_is_vulkan_api    = false;

    LogToFile(L"[Device]: Device destroyed\n");
}

//static void init_swapchain(reshade::api::swapchain* swapchain)
//{
//    static int t = 0; ++t;
//}

// ============================================================================
// [ACTIVE MINIMAL PIPELINE / 핵심 동작 구역]
// ----------------------------------------------------------------------------
// [English Description]:
// - Functionality: on_create_swapchain()
// - Purpose      : Enforces FP16 format, FLIP_DISCARD swap effect, and AT LEAST 2 buffers.
//                  [ANTI-INVERSION NOTICE]: We enforce "AT LEAST 2 buffers" (not "maximum 2"),
//                  which is the strict requirement for DXGI Flip Model and FP16 presentation.
//                  This creates the swapchain natively in 16-bit, completely eliminating 
//                  the need for runtime ResizeBuffers calls that conflict with version.dll.
// ----------------------------------------------------------------------------
// [한국어 상세 설명 (정밀 대조 번역)]:
// - 동작 기능: on_create_swapchain()
// - 목적: FP16 포맷, FLIP_DISCARD 스왑 모델 및 최소 2개 이상의 버퍼를 강제 적용합니다.
//         [의미 반전 방지 주의]: 버퍼 개수를 '최대 2개로 제한'하는 것이 아니라 
//         DXGI 플립 모델의 필수 규격인 '최소 2개 이상으로 보장'하는 것입니다.
//         스왑체인을 처음부터 16비트로 생성하여, 모드 매니저(version.dll)와 충돌하는
//         런타임 ResizeBuffers 호출 필요성을 원천적으로 제거합니다.
// ============================================================================
static bool on_create_swapchain(reshade::api::device_api api, reshade::api::swapchain_desc& swapchain_desc, void* hwnd)
{
    if (g_is_supported_api)
    {
        LogToFile(L"[create_swapchain]: Original Buffers: %u, OrigFormat: %hs, PresentMode: %u\n",
            swapchain_desc.back_buffer_count,
            GetDxgiFormatName(static_cast<DXGI_FORMAT>(swapchain_desc.back_buffer.texture.format)),
            swapchain_desc.present_mode);

        // 1) Enforce 16-bit floating point format / 16비트 부동소수점 포맷 강제
        swapchain_desc.back_buffer.texture.format = reshade::api::format::r16g16b16a16_float;

        if (g_use_hdr10)
        {
            swapchain_desc.back_buffer.texture.format = reshade::api::format::r10g10b10a2_unorm;
        }

        //swapchain_desc.refresh_rate.numerator = 60;
        //swapchain_desc.refresh_rate.denominator = 1;

        // 2) Enforce AT LEAST 2 buffers (Strict DXGI requirement for Flip Model)
        // [주의]: '최대 2개'가 아니라 플립 모델에 필요한 '최소 2개 이상' 보장
        if (swapchain_desc.back_buffer_count < 2)
        {
            swapchain_desc.back_buffer_count = 2;
        }

        // 3) Enforce Flip Model and Tearing flag for windowed/fullscreen stability
        // 플립 모델 및 창 모드/전체화면 안정성을 위한 ALLOW_TEARING 플래그 적용
        swapchain_desc.present_mode   = static_cast<uint32_t>(DXGI_SWAP_EFFECT_FLIP_DISCARD);
        swapchain_desc.present_flags |= static_cast<uint32_t>(DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING);

        LogToFile(L"[create_swapchain]: Enforced FP16, BufferCount >= 2, FLIP_DISCARD (HWND: 0x%p)\n", hwnd);
    }

    return true;
}

static void on_init_swapchain(reshade::api::swapchain* swapchain, bool resize)
{
    if (g_is_supported_api)
    {
        const std::lock_guard<std::mutex> lock(g_mutex);

        reshade::api::device* const device = swapchain->get_device();
        if (device == nullptr)
        {
            LogToFile(L"[init_swapchain]: Failed to obtain device\n");
            return;
        }

        for (uint32_t i = 0; i < swapchain->get_back_buffer_count(); ++i)
        {
            const reshade::api::resource buffer = swapchain->get_back_buffer(i);

            g_back_buffers.emplace(buffer.handle);
        }

        LogToFile(L"[init_swapchain]: Tracked %u back buffers\n", swapchain->get_back_buffer_count());

        // ============================================================================
        // [ACTIVE MINIMAL PIPELINE / 핵심 동작 구역]
        // ----------------------------------------------------------------------------
        // [English Description]:
        // - Functionality: Safe QueryInterface and single-shot scRGB color space binding.
        // - Mechanism    : Safely obtains IDXGISwapChain3 via COM QueryInterface to avoid
        //                  vtable conflicts with version.dll. Applies scRGB exactly ONCE.
        // ----------------------------------------------------------------------------
        // [한국어 상세 설명 (정밀 대조 번역)]:
        // - 동작 기능: 안전한 QueryInterface 조회 및 단 1회의 scRGB 색 공간 바인딩.
        // - 동작 원리: 모드 매니저(version.dll)의 가상 함수 훅과의 충돌을 피하기 위해
        //             표준 COM QueryInterface로 IDXGISwapChain3를 안전하게 획득하고,
        //             scRGB 색 공간 설정을 정확히 1회만 호출합니다.
        // ============================================================================
        IDXGISwapChain* native_swapchain = reinterpret_cast<IDXGISwapChain*>(swapchain->get_native());
        if (native_swapchain == nullptr)
        {
            LogToFile(L"[init_swapchain]: Native swapchain pointer is null\n");
            return;
        }

        ATL::CComPtr<IDXGISwapChain3> swapchain3;
        if (SUCCEEDED(native_swapchain->QueryInterface(__uuidof(IDXGISwapChain3), (void**)&swapchain3)))
        {
            // Apply scRGB directly without crash-inducing display check loops
            // 충돌을 유발하는 디스플레이 검사 루프 없이 scRGB를 직접 안전하게 적용
            dxgi_swapchain_color_space(swapchain3, DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709);
        }
        else
        {
            LogToFile(L"[init_swapchain]: Failed to QueryInterface IDXGISwapChain3 (Graceful Fallback)\n");
        }

        /* ============================================================================
         * [DISABLED / PRESERVED ORIGINAL CODE / 비활성화 및 원본 보존 구역]
         * ----------------------------------------------------------------------------
         * [English Description]:
         * - Original Code: Display check via dxgi_check_display_hdr_support() and 
         *                  runtime ResizeBuffers() loops (Lines 305~431).
         * - Reason for Disabling: 
         *   1) dxgi_check_display_hdr_support strictly requires HDR10 (G2084) and fails on scRGB.
         *   2) Calling ResizeBuffers inside on_init_swapchain while Dragon Engine and 
         *      version.dll hold back buffer RTV references triggers DXGI_ERROR_INVALID_CALL 
         *      and immediately crashes the game to desktop (CTD).
         * - Revival Potential: Preserved 100% line-by-line for historical reference.
         * ----------------------------------------------------------------------------
         * [한국어 상세 설명 (정밀 대조 번역)]:
         * - 원본 코드: dxgi_check_display_hdr_support() 검사 및 런타임 ResizeBuffers() 루프 전체.
         * - 비활성화 이유:
         *   1) 디스플레이 검사 함수는 HDR10(G2084)만을 요구하여 scRGB 환경에서 실패를 유발합니다.
         *   2) 드래곤 엔진과 모드 매니저(version.dll)가 이미 백버퍼 뷰(RTV)를 쥐고 있는 상태에서
         *      on_init_swapchain 내에서 ResizeBuffers를 부르면 DXGI_ERROR_INVALID_CALL 에러가
         *      발생하여 게임이 시작 즉시 튕기기 때문입니다.
         * - 복구 가능성: 원본 보존 원칙에 따라 단 한 줄도 지우지 않고 전량 보존합니다.
         * ============================================================================ */
        /*
        ATL::CComPtr<IDXGISwapChain4> swapchain4;

        if (SUCCEEDED(native_swapchain->QueryInterface(__uuidof(IDXGISwapChain4), (void**)&swapchain4)))
        {
            if (g_hdr_support == false)
            {
#ifdef __WINRT__
                IDXGIFactory2* factory = nullptr;
                if (FAILED(swapchain4->GetParent(__uuidof(IDXGIFactory2), (void**)&factory)))
                {
                    reshade::log::message(reshade::log::level::error, "[DXGI]: Failed to get the swap chain's factory 2");
                    return;
                }

                g_hdr_support = dxgi_check_display_hdr_support(factory, reinterpret_cast<HWND>(swapchain->get_hwnd()));
#else
                IDXGIFactory1* factory = nullptr;
                if (FAILED(swapchain4->GetParent(__uuidof(IDXGIFactory1), (void**)&factory)))
                {
                    reshade::log::message(reshade::log::level::error, "[DXGI]: Failed to get the swap chain's factory 1");
                    return;
                }

                g_hdr_support = dxgi_check_display_hdr_support(factory, reinterpret_cast<HWND>(swapchain->get_hwnd()));

                factory->Release();
#endif // __WINRT__
            }

            if (g_hdr_support == false)
            {
                reshade::log::message(reshade::log::level::error, "[DXGI]: Failed as no HDR support");
                return;
            }

            if (g_hdr_enable == true)
            {
                DXGI_SWAP_CHAIN_DESC1 desc;
                if (FAILED(swapchain4->GetDesc1(&desc)))
                {
                    reshade::log::message(reshade::log::level::error, "[DXGI]: Failed to get swap chain description");
                    return;
                }

                if (g_first_csp_change)
                {
                    g_original_format  = desc.Format;
                    g_first_csp_change = false;
                }

                DXGI_FORMAT           new_swapchain_format = DXGI_FORMAT_R16G16B16A16_FLOAT;
                DXGI_COLOR_SPACE_TYPE new_colour_space     = DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709;

                if (g_use_hdr10)
                {
                    new_swapchain_format = DXGI_FORMAT_R10G10B10A2_UNORM;
                    new_colour_space     = DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020;
                }

                if (new_swapchain_format != desc.Format
                 || new_colour_space     != g_colour_space)
                {
                    HRESULT hr = swapchain4->ResizeBuffers(
                        desc.BufferCount,
                        desc.Width,
                        desc.Height,
                        new_swapchain_format,
                        desc.Flags);

                    if (hr == DXGI_ERROR_INVALID_CALL)
                    {
                        std::stringstream log_str;
                        log_str << "[DXGI]: Failed to resize swap chain buffers "
                                << EnumerateDxgiFormat(new_swapchain_format).c_str()
                                << ": error DXGI_ERROR_INVALID_CALL";
                        reshade::log::message(reshade::log::level::error, log_str.str().c_str());
                    }
                    else if (FAILED(hr))
                    {
                        std::stringstream log_str;
                        log_str << "[DXGI]: Failed to resize swap chain buffers "
                                << EnumerateDxgiFormat(new_swapchain_format).c_str()
                                << ": error 0x"
                                << std::hex
                                << hr;
                        reshade::log::message(reshade::log::level::error, log_str.str().c_str());
                        return;
                    }

                    std::stringstream log_str;
                    log_str << "[DXGI]: swap chain format updated to "
                            << EnumerateDxgiFormat(new_swapchain_format).c_str();
                    reshade::log::message(reshade::log::level::info, log_str.str().c_str());
                }

                dxgi_swapchain_color_space(swapchain4, new_colour_space);
            }
            else if (g_hdr_enable == false)
            {
                DXGI_SWAP_CHAIN_DESC1 desc;
                if (FAILED(swapchain4->GetDesc1(&desc)))
                {
                    reshade::log::message(reshade::log::level::error, "[DXGI]: Failed to get swap chain description");
                    return;
                }

                if (g_first_csp_change)
                {
                    g_original_format  = desc.Format;
                    g_first_csp_change = false;
                }

                DXGI_FORMAT           new_swapchain_format = g_original_format;
                DXGI_COLOR_SPACE_TYPE new_colour_space     = DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;

                if (new_swapchain_format != desc.Format
                 || new_colour_space     != g_colour_space)
                {
                    HRESULT hr = swapchain4->ResizeBuffers(
                        desc.BufferCount,
                        desc.Width,
                        desc.Height,
                        new_swapchain_format,
                        desc.Flags);

                    if (hr == DXGI_ERROR_INVALID_CALL)
                    {
                        std::stringstream log_str;
                        log_str << "[DXGI]: Failed to resize swap chain buffers "
                                << EnumerateDxgiFormat(new_swapchain_format).c_str()
                                << ": error DXGI_ERROR_INVALID_CALL";
                        reshade::log::message(reshade::log::level::error, log_str.str().c_str());
                    }
                    else if (FAILED(hr))
                    {
                        std::stringstream log_str;
                        log_str << "[DXGI]: Failed to resize swap chain buffers "
                                << EnumerateDxgiFormat(new_swapchain_format).c_str()
                                << ": error 0x"
                                << std::hex
                                << hr;
                        reshade::log::message(reshade::log::level::error, log_str.str().c_str());
                    }

                    std::stringstream log_str;
                    log_str << "[DXGI]: swap chain format updated to "
                            << EnumerateDxgiFormat(new_swapchain_format).c_str();
                    reshade::log::message(reshade::log::level::info, log_str.str().c_str());
                }

                dxgi_swapchain_color_space(swapchain4, new_colour_space);
            }
        }
        */
    }
}

static void on_destroy_swapchain(reshade::api::swapchain* swapchain, bool resize)
{
    if (g_is_supported_api)
    {
        const std::lock_guard<std::mutex> lock(g_mutex);

        reshade::api::device* const device = swapchain->get_device();

        for (uint32_t i = 0; i < swapchain->get_back_buffer_count(); ++i)
        {
            const reshade::api::resource buffer = swapchain->get_back_buffer(i);

            g_back_buffers.erase(buffer.handle);
        }

        LogToFile(L"[destroy_swapchain]: Cleared back buffer tracking\n");
    }
}

// ============================================================================
// [ACTIVE MINIMAL PIPELINE / 핵심 동작 구역]
// ----------------------------------------------------------------------------
// [English Description]:
// - Functionality: on_create_resource_view()
// - Purpose      : Intercepts RTV creation on the back buffer.
//                  If the Dragon Engine requests an R8G8B8A8 view description on the 
//                  FP16 back buffer, Direct3D 11 returns E_INVALIDARG and crashes.
//                  This callback coerces the descriptor format to r16g16b16a16_float,
//                  preventing the engine crash and ensuring safe view binding.
// ----------------------------------------------------------------------------
// [한국어 상세 설명 (정밀 대조 번역)]:
// - 동작 기능: on_create_resource_view()
// - 목적: 백버퍼에 대한 렌더 타깃 뷰(RTV) 생성을 가로챕니다.
//         드래곤 엔진이 FP16 백버퍼에 대해 기존 8비트(R8G8B8A8) 뷰 생성을 요청하면
//         D3D11이 규격 불일치(E_INVALIDARG)로 크래시를 냅니다.
//         이 콜백은 뷰의 요청 포맷을 백버퍼와 동일한 r16g16b16a16_float로 교정하여
//         엔진 크래시를 완벽히 방지하고 안전하게 뷰가 바인딩되도록 보장합니다.
// ============================================================================
static bool on_create_resource_view(reshade::api::device* device, reshade::api::resource resource, reshade::api::resource_usage usage_type, reshade::api::resource_view_desc& desc)
{
    if (g_is_supported_api)
    {
        if ((desc.format != reshade::api::format::unknown) && device)
        {
            bool is_back_buffer = false;

            {
                const std::lock_guard<std::mutex> lock(g_mutex);
                for (uint64_t back_buffer : g_back_buffers)
                {
                    if (resource == back_buffer)
                    {
                        is_back_buffer = true;
                        break;
                    }
                }
            }

            if (is_back_buffer)
            {
                const reshade::api::resource_desc texture_desc = device->get_resource_desc(resource);

                if (texture_desc.texture.format == reshade::api::format::r10g10b10a2_unorm)
                {
                    desc.format = reshade::api::format::r10g10b10a2_unorm;
                    return true;
                }

                if (texture_desc.texture.format == reshade::api::format::r16g16b16a16_float)
                {
                    desc.format = reshade::api::format::r16g16b16a16_float;
                    return true;
                }
            }
        }
    }

    return false;
}

/* ============================================================================
 * [DISABLED / PRESERVED ORIGINAL CODE / 비활성화 및 원본 보존 구역]
 * ----------------------------------------------------------------------------
 * [English Description]:
 * - Original Function: draw_settings_overlay()
 * - Reason for Disabling: Eliminates UI rendering overhead and potential overlay 
 *                         hook clashes with version.dll and the Dragon Engine.
 * - Revival Potential   : Preserved 100% for future in-game GUI integration.
 * ----------------------------------------------------------------------------
 * [한국어 상세 설명 (정밀 대조 번역)]:
 * - 원본 함수: draw_settings_overlay()
 * - 비활성화 이유: UI 렌더링 오버헤드를 없애고, 모드 매니저(version.dll) 및 드래곤 엔진과의
 *                 오버레이 후킹 충돌 가능성을 원천 차단하기 위해 비활성화합니다.
 * - 복구 가능성: 추후 인게임 설정 GUI 재도입 시 참조를 위해 100% 보존합니다.
 * ============================================================================ */
/*
static void draw_settings_overlay(reshade::api::effect_runtime* runtime)
{
    if (g_is_supported_api)
    {
        if (g_hdr_support)
        {
            if (g_is_vulkan_api)
            {
                ImGui::TextColored(ImVec4(1.f, 0.3f, 0.3f, 1.f),
                                   "Vulkan API supported is experimental!!"
                              "\n" "Do not use this with dxvk! Use my dxvk HDR-mod instead!");
            }

            bool hdr_enable_modified    = false;
            bool hdr_use_hdr10_modified = false;

            hdr_enable_modified    |= ImGui::Checkbox("Enable HDR", &g_hdr_enable);
            hdr_use_hdr10_modified |= ImGui::Checkbox("Use HDR10 instead of scRGB (needs game restart or chaning the resolution of the game)", &g_use_hdr10);

            if (hdr_enable_modified)
            {
                reshade::set_config_value(g_runtime, "HDR", "EnableHDR", g_hdr_enable);
            }
            if (hdr_use_hdr10_modified)
            {
                reshade::set_config_value(g_runtime, "HDR", "UseHDR10", g_use_hdr10);
            }
        }
        else
        {
            ImGui::TextUnformatted("HDR support is not enabled. If hardware can support it please go to Windows 'Display Settings' and then turn on 'Use HDR'");
        }
    }
    else
    {
        ImGui::TextUnformatted("Unsupported API!");
    }
}
*/

static void on_init_effect_runtime(reshade::api::effect_runtime* runtime)
{
    if (g_is_supported_api)
    {
        g_runtime = runtime;

        set_reshade_colour_space();

        LogToFile(L"[Runtime]: Effect runtime initialized, color space refreshed\n");
    }
}

static void on_destroy_effect_runtime(reshade::api::effect_runtime* runtime)
{
    if (g_is_supported_api)
    {
        g_runtime = nullptr;
        LogToFile(L"[Runtime]: Effect runtime destroyed\n");
    }
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        // Call 'reshade::register_addon()' before you call any other function of the ReShade API.
        // This will look for the ReShade instance in the current process and initialize the API when found.
        if (!reshade::register_addon(hinstDLL))
            return FALSE;

        reshade::log::message(reshade::log::level::info, "DLL attached");
        reshade::log::message(reshade::log::level::info, "ReShade addon registered");
        LogToFile(L"[DllMain]: ReShade FP16 scRGB Addon attached successfully\n");

        /* ============================================================================
         * [DISABLED / PRESERVED ORIGINAL CODE / 비활성화 및 원본 보존 구역]
         * ----------------------------------------------------------------------------
         * [English Description]:
         * - Original Code       : reshade::register_overlay(nullptr, draw_settings_overlay);
         * - Reason for Disabling: Overlay UI disabled to prevent UI-level hook interference.
         * ----------------------------------------------------------------------------
         * [한국어 상세 설명 (정밀 대조 번역)]:
         * - 원본 코드: reshade::register_overlay(nullptr, draw_settings_overlay);
         * - 비활성화 이유: UI 레벨의 후킹 간섭을 차단하기 위해 오버레이 등록을 비활성화합니다.
         * ============================================================================ */
        /*
        reshade::register_overlay(nullptr, draw_settings_overlay);
        */

        reshade::register_event<reshade::addon_event::create_swapchain>(&on_create_swapchain);
        reshade::register_event<reshade::addon_event::init_swapchain>(on_init_swapchain);
        reshade::register_event<reshade::addon_event::destroy_swapchain>(on_destroy_swapchain);

        reshade::register_event<reshade::addon_event::create_resource_view>(&on_create_resource_view);

        reshade::register_event<reshade::addon_event::init_effect_runtime>(on_init_effect_runtime);
        reshade::register_event<reshade::addon_event::destroy_effect_runtime>(on_destroy_effect_runtime);

        reshade::register_event<reshade::addon_event::init_device>(&on_init_device);
        reshade::register_event<reshade::addon_event::destroy_device>(&on_destroy_device);

        break;

    case DLL_PROCESS_DETACH:
        LogToFile(L"[DllMain]: ReShade FP16 scRGB Addon detaching\n");

        reshade::unregister_event<reshade::addon_event::create_swapchain>(&on_create_swapchain);
        reshade::unregister_event<reshade::addon_event::init_swapchain>(on_init_swapchain);
        reshade::unregister_event<reshade::addon_event::destroy_swapchain>(on_destroy_swapchain);

        reshade::unregister_event<reshade::addon_event::create_resource_view>(&on_create_resource_view);

        reshade::unregister_event<reshade::addon_event::init_effect_runtime>(on_init_effect_runtime);
        reshade::unregister_event<reshade::addon_event::destroy_effect_runtime>(on_destroy_effect_runtime);

        reshade::unregister_event<reshade::addon_event::init_device>(&on_init_device);
        reshade::unregister_event<reshade::addon_event::destroy_device>(&on_destroy_device);

        // And finally unregister the add-on from ReShade (this will automatically unregister any events and overlays registered by this add-on too).
        reshade::unregister_addon(hinstDLL);
        break;
    }
    return TRUE;
}
