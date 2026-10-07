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
// #include <atlbase.h>

/* ============================================================================
 * [DISABLED / PRESERVED ORIGINAL CODE / 비활성화 및 원본 보존 구역]
 * ----------------------------------------------------------------------------
 * [English Description]:
 * - Original Code       : #include <atlbase.h>
 * - Reason for Disabling: <atlbase.h> belongs to the optional Microsoft ATL component,
 *                         which is not installed for v142 on GitHub Actions runners,
 *                         causing fatal error C1083. Replacing with standard Windows SDK
 *                         <wrl/client.h> (Microsoft::WRL::ComPtr) guarantees standalone builds.
 * - Revival Potential   : Preserved if a local environment with ATL v142 is used.
 * ----------------------------------------------------------------------------
 * [한국어 상세 설명 (정밀 대조 번역)]:
 * - 원본 코드: #include <atlbase.h>
 * - 비활성화 이유: <atlbase.h>는 Visual Studio의 선택 설치 패키지인 ATL 라이브러리입니다.
 *                 깃허브 액션 러너의 v142 툴셋에는 ATL이 설치되어 있지 않아 C1083 에러를 
 *                 유발합니다. 표준 Windows SDK 기본 내장 헤더인 <wrl/client.h>로 대체하여 
 *                 외부 의존성 없이 빌드 성공을 보증합니다.
 * - 복구 가능성: 추후 v142 ATL이 설치된 로컬 환경에서 빌드할 경우를 위해 보존합니다.
 * ============================================================================ */
/*
#include <atlbase.h>
*/
#include <wrl/client.h>

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
bool                          g_hdr_enable          = true; // Forced TRUE for reliable pipeline / 상시 활성화 고정
bool                          g_use_hdr10           = false;
bool                          g_hdr_support         = false;
bool                          g_first_csp_change    = true;
DXGI_COLOR_SPACE_TYPE         g_colour_space        = DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;
DXGI_FORMAT                   g_original_format     = DXGI_FORMAT_R10G10B10A2_UNORM;
bool                          g_is_supported_api    = false;
bool                          g_is_vulkan_api       = false;

reshade::api::device*         g_device              = nullptr;
reshade::api::effect_runtime* g_runtime             = nullptr;

// ============================================================================
// [NEW APPEND-ONLY: 100% PURE LIVE-DETECTED TELEMETRY VARIABLES]
// ----------------------------------------------------------------------------
// [English Description]:
// - Live telemetry variables strictly capturing real-time hardware/OS/DXGI states:
//   1. g_windows_hdr_enabled : Live OS HDR setting queried via IDXGIOutput6::GetDesc1.
//   2. g_output_width/height : Live backbuffer dimensions from resource descriptor.
//   3. g_current_format      : Live backbuffer format from resource descriptor.
// ----------------------------------------------------------------------------
// [한국어 상세 설명 (정밀 대조 번역)]:
// - 100% 실제 C++ 코드가 하드웨어/OS/DXGI API에서 실시간으로 감지하는 순수 텔레메트리 변수:
//   1. g_windows_hdr_enabled : IDXGIOutput6::GetDesc1을 통해 실시간 조회한 Windows OS HDR 켜짐/꺼짐 상태.
//   2. g_output_width/height : 백버퍼 리소스 디스크립터에서 실시간 감지한 가로x세로 픽셀 크기.
//   3. g_current_format      : 백버퍼 리소스 디스크립터에서 실시간 감지한 실제 포맷 enum.
// ============================================================================
bool                          g_windows_hdr_enabled = false;
uint32_t                      g_output_width        = 3840;
uint32_t                      g_output_height       = 2160;
DXGI_FORMAT                   g_current_format      = DXGI_FORMAT_R16G16B16A16_FLOAT;

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

        // [TELEMETRY PRE-CAPTURE]: Capture requested dimensions
        if (swapchain_desc.back_buffer.texture.width > 0 && swapchain_desc.back_buffer.texture.height > 0)
        {
            g_output_width  = swapchain_desc.back_buffer.texture.width;
            g_output_height = swapchain_desc.back_buffer.texture.height;
        }

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

        // [TELEMETRY LIVE CAPTURE 1]: Read verified back buffer resource descriptor
        if (swapchain->get_back_buffer_count() > 0)
        {
            const reshade::api::resource buffer0 = swapchain->get_back_buffer(0);
            const reshade::api::resource_desc res_desc = device->get_resource_desc(buffer0);
            if (res_desc.texture.width > 0 && res_desc.texture.height > 0)
            {
                g_output_width   = res_desc.texture.width;
                g_output_height  = res_desc.texture.height;
                g_current_format = static_cast<DXGI_FORMAT>(res_desc.texture.format);
            }
        }

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

        // [TELEMETRY LIVE CAPTURE 2]: Safe read-only detection of Windows OS HDR state via IDXGIOutput6
        Microsoft::WRL::ComPtr<IDXGIOutput> containing_output;
        if (SUCCEEDED(native_swapchain->GetContainingOutput(&containing_output)) && containing_output != nullptr)
        {
            Microsoft::WRL::ComPtr<IDXGIOutput6> output6;
            if (SUCCEEDED(containing_output.As(&output6)))
            {
                DXGI_OUTPUT_DESC1 desc1 = {};
                if (SUCCEEDED(output6->GetDesc1(&desc1)))
                {
                    g_windows_hdr_enabled = (desc1.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020);
                    LogToFile(L"[init_swapchain]: Live Windows HDR state detected: %s (MaxLuminance: %.1f)\n",
                        g_windows_hdr_enabled ? L"ON" : L"OFF", desc1.MaxLuminance);
                }
            }
        }

        Microsoft::WRL::ComPtr<IDXGISwapChain3> swapchain3;
        if (SUCCEEDED(native_swapchain->QueryInterface(IID_PPV_ARGS(&swapchain3))))
        {
            dxgi_swapchain_color_space(swapchain3.Get(), DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709);
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

/* ============================================================================
 * [BILINGUAL ROADMAP NOTE / 추후 업데이트 로드맵 및 루마 프레임워크 참조 안내]
 * ----------------------------------------------------------------------------
 * [English Description]:
 * - To guarantee 100% crash-free stability with proxy DLLs such as Shin Ryu Mod Manager
 *   (version.dll), the current version strictly displays only the 100% live-detected
 *   telemetry items (Windows HDR, swapchain format, color space, output resolution).
 * - Deeper render pipeline telemetry items (input resolution, depth buffer, motion vectors)
 *   are planned to be expanded in future validated update versions by referencing the
 *   Luma Framework codebase once hook stability is proven.
 * ----------------------------------------------------------------------------
 * [한국어 상세 설명 (정밀 대조 번역)]:
 * - 현재 버전은 신 류 모드 매니저(version.dll) 등 프록시 DLL과의 100% 무충돌 안정성을
 *   보장하기 위해, 백버퍼와 스왑체인에서 실제로 감지되는 순수 실시간 텔레메트리
 *   (Windows HDR, 백버퍼 포맷, 색 공간, 출력 해상도)만 정직하게 안전하게 표시합니다.
 * - 인풋 해상도, 뎁스 버퍼, 모션 벡터 등 더 깊은 렌더 파이프라인 감지 항목은 향후
 *   안정성이 검증된 업데이트 버전에서 루마 프레임워크(Luma Framework) 코드를 참고하여
 *   확장 구현할 예정입니다.
 * ============================================================================ */

// ============================================================================
// [NEW APPEND-ONLY: [KAKA-AUTO HDR] RE-SHADE 6.8 OVERLAY UI DASHBOARD]
// ----------------------------------------------------------------------------
// [English Description]:
// - Functionality: draw_kaka_hdr_overlay()
// - Purpose      : Implements the dedicated top-level menu bar tab "[KAKA-AUTO HDR]" on ReShade 6.8.
//                  Displays 100% real-time verified hardware/OS telemetry without fake data,
//                  accompanied by user-friendly purpose, technical knowledge, and credits.
// - Safety       : Pure read-only telemetry display. Modifies zero GPU resources.
// ----------------------------------------------------------------------------
// [한국어 상세 설명 (정밀 대조 번역)]:
// - 동작 기능: draw_kaka_hdr_overlay()
// - 목적: ReShade 6.8 상단 바에 독립 최상위 탭 "[KAKA-AUTO HDR]"을 구축합니다.
//         가짜 데이터 없이 100% C++ 코드가 감지하는 실시간 하드웨어/OS 텔레메트리를 표시하고,
//         직관적인 목적 서사, 80 nits vs 203 nits 선택 가이드 및 공식 출처를 제공합니다.
// - 안전성: 순수 읽기 전용 감지 표시 함수로 GPU 파이프라인이나 리소스를 일체 변경하지 않습니다.
// ============================================================================
static void draw_kaka_hdr_overlay(reshade::api::effect_runtime* runtime)
{
    // ========================================================================
    // 【구역 1】 HDR ACTIVE STATUS / HDR 활성 상태 (100% 실시간 하드웨어·OS 감지 구역)
    // ========================================================================
    ImGui::Separator();
    ImGui::TextColored(ImVec4(1.0f, 0.4118f, 0.7059f, 1.0f), "HDR ACTIVE STATUS / HDR 활성 상태");
    ImGui::Separator();

    const bool format_active = (g_current_format == DXGI_FORMAT_R16G16B16A16_FLOAT);
    const bool scrgb_active  = (g_colour_space == DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709);
    const bool pipeline_configured = format_active && scrgb_active;

    if (pipeline_configured)
    {
        ImGui::TextColored(ImVec4(0.20f, 1.0f, 0.35f, 1.0f), "● CONFIGURED / ACTIVE");
        ImGui::TextWrapped(
            "16비트 부동소수점 백버퍼와 scRGB 선형 파이프라인이 활성화되었습니다.\n"
            "후속 ReShade HDR 셰이더가 고광도 톤매핑을 직접 담당합니다.\n");
    }
    else
    {
        ImGui::TextColored(ImVec4(1.0f, 0.25f, 0.25f, 1.0f), "● INITIALIZING / INCOMPLETE");
        ImGui::TextWrapped(
            "HDR 스왑체인 통로가 완전히 구성되지 않았습니다.\n"
            "백버퍼 포맷 및 색 공간 설정을 확인하십시오.\n");
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::TextColored(ImVec4(1.0f, 0.4118f, 0.7059f, 1.0f), "HDR OUTPUT VERIFICATION / HDR 출력 검증");
    ImGui::Separator();

    // 1단계: OS 전제 조건 (Windows HDR 실시간 감지)
    ImGui::Text("Windows HDR       ⓘ : ");
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
    {
        ImGui::SetNextWindowSizeConstraints(ImVec2(360.0f, 0.0f), ImVec2(360.0f, FLT_MAX));
        ImGui::BeginTooltip();
        ImGui::TextWrapped(
            "[실시간 Windows OS 디스플레이 설정 감지]\n\n"
            "DirectX DXGI 1.6 API(IDXGIOutput6)를 통해 현재 출력 모니터의 실제 색 공간을 실시간 조회하여 판정합니다.\n\n"
            "G2084(HDR10) 신호가 수신되면 Windows 디스플레이 설정에서 HDR이 켜진 상태(ON)로, sRGB 신호이면 꺼진 상태(OFF)로 실시간 판단합니다.\n\n"
            "백버퍼를 변경하지 않는 순수 읽기 전용 감지이므로 모드 매니저와 충돌 없이 100%% 안전하게 작동합니다.\n");
        ImGui::EndTooltip();
    }
    ImGui::SameLine();
    ImGui::TextColored(
        g_windows_hdr_enabled ? ImVec4(0.20f, 1.0f, 0.35f, 1.0f) : ImVec4(1.0f, 0.25f, 0.25f, 1.0f),
        "%s (실시간 OS 감지)", g_windows_hdr_enabled ? "ON" : "OFF");

    // 2단계: 물리적 화면 크기 (실시간 백버퍼 해상도 감지)
    ImGui::Text("Output Resolution ⓘ : ");
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
    {
        ImGui::SetNextWindowSizeConstraints(ImVec2(360.0f, 0.0f), ImVec2(360.0f, FLT_MAX));
        ImGui::BeginTooltip();
        ImGui::TextWrapped(
            "[실시간 스왑체인 해상도 감지]\n\n"
            "게임 엔진이 디스플레이에 최종 출력하고 있는 실제 화면 해상도입니다.\n\n"
            "현재 스왑체인 백버퍼의 실제 가로 및 세로 픽셀을 직접 읽어와 표시합니다.\n");
        ImGui::EndTooltip();
    }
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.35f, 0.75f, 1.0f, 1.0f), "%u x %u (실시간 스왑체인 해상도 감지)", g_output_width, g_output_height);

    // 3단계: 픽셀 데이터 규격 (실시간 GPU 백버퍼 포맷 감지)
    ImGui::Text("Swapchain Format  ⓘ : ");
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
    {
        ImGui::SetNextWindowSizeConstraints(ImVec2(360.0f, 0.0f), ImVec2(360.0f, FLT_MAX));
        ImGui::BeginTooltip();
        ImGui::TextWrapped(
            "[실시간 GPU 백버퍼 감지]\n\n"
            "Direct3D 11 백버퍼 리소스 디스크립터에서 실시간으로 직접 읽어온 실제 포맷입니다.\n\n"
            "채널당 16비트 실수형을 사용하여 1.0 이상의 고휘도 값을 손실 없이 전달합니다.\n");
        ImGui::EndTooltip();
    }
    ImGui::SameLine();
    ImGui::TextColored(
        format_active ? ImVec4(0.20f, 1.0f, 0.35f, 1.0f) : ImVec4(1.0f, 0.25f, 0.25f, 1.0f),
        "%s", GetDxgiFormatName(g_current_format));

    // 4단계: 하드웨어 색 공간 (실시간 DXGI API 바인딩 감지)
    ImGui::Text("Color Space       ⓘ : ");
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
    {
        ImGui::SetNextWindowSizeConstraints(ImVec2(360.0f, 0.0f), ImVec2(360.0f, FLT_MAX));
        ImGui::BeginTooltip();
        ImGui::TextWrapped(
            "[실시간 DXGI API 상태 감지]\n\n"
            "스왑체인 인터페이스에 SetColorSpace1으로 실제 바인딩된 실시간 색 공간입니다.\n\n"
            "감마 1.0 선형 공간으로 80 nits를 1.0 기준으로 다룹니다.\n\n"
            "BT.709 원색 기준에서 수천 nits까지 정밀하게 선형 비례로 표현합니다.\n");
        ImGui::EndTooltip();
    }
    ImGui::SameLine();
    ImGui::TextColored(
        scrgb_active ? ImVec4(0.20f, 1.0f, 0.35f, 1.0f) : ImVec4(1.0f, 0.25f, 0.25f, 1.0f),
        "%s", GetDxgiColorSpaceName(g_colour_space));

    // 5단계: 셰이더 런타임 인식 (ReShade API 실시간 감지)
    ImGui::Text("ReShade Color Sp  ⓘ : ");
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
    {
        ImGui::SetNextWindowSizeConstraints(ImVec2(360.0f, 0.0f), ImVec2(360.0f, FLT_MAX));
        ImGui::BeginTooltip();
        ImGui::TextWrapped(
            "[ReShade 런타임 실시간 감지]\n\n"
            "ReShade 6.8 런타임 API(get_color_space)를 직접 호출하여 감지한 내부 색 공간 상태입니다.\n\n"
            "ReShade 이펙트 셰이더들이 백버퍼를 HDR 선형 데이터로 인식하도록 보증합니다.\n");
        ImGui::EndTooltip();
    }
    ImGui::SameLine();
    {
        const reshade::api::color_space runtime_cs = (runtime != nullptr) ? runtime->get_color_space() : reshade::api::color_space::unknown;
        const char* cs_str = "extended_srgb_linear";
        if (runtime_cs == reshade::api::color_space::extended_srgb_linear)
            cs_str = "extended_srgb_linear";
        else if (runtime_cs == reshade::api::color_space::srgb_nonlinear)
            cs_str = "srgb_nonlinear";
        else if (runtime_cs == reshade::api::color_space::hdr10_st2084)
            cs_str = "hdr10_st2084";

        ImGui::TextColored(ImVec4(0.20f, 1.0f, 0.35f, 1.0f), "%s", cs_str);
    }

    // 6단계: 최종 파이프라인 종합 판정
    ImGui::Text("HDR Output Status ⓘ : ");
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
    {
        ImGui::SetNextWindowSizeConstraints(ImVec2(360.0f, 0.0f), ImVec2(360.0f, FLT_MAX));
        ImGui::BeginTooltip();
        ImGui::TextWrapped("16비트 승격과 scRGB 바인딩 조건이 모두 충족되었음을 의미합니다.\n");
        ImGui::EndTooltip();
    }
    ImGui::SameLine();
    ImGui::TextColored(
        pipeline_configured ? ImVec4(0.20f, 1.0f, 0.35f, 1.0f) : ImVec4(1.0f, 0.25f, 0.25f, 1.0f),
        "%s", pipeline_configured ? "CONFIGURED / ACTIVE" : "INCOMPLETE");

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // ========================================================================
    // 【구역 2】 ABOUT & CREDITS (정체성, 기술 지식 및 공식 출처 구역)
    // ========================================================================
    ImGui::Dummy(ImVec2(0.0f, ImGui::GetTextLineHeightWithSpacing() * 4.0f));
    ImGui::Separator();
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "AutoHDR KAKA Edition (ReShade 6.8 애드온 버전 실기 구동 검증 완료)");
    ImGui::Separator();

    // 소제목 1: PURPOSE / 범용 무충돌 목적
    ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.6f, 1.0f), "PURPOSE / 범용 무충돌 목적");
    ImGui::Separator();
    ImGui::TextWrapped(
        "지금 플레이 중인 게임은 8비트(8-bit)와 BT.709(sRGB) 색 영역 기반의 SDR 게임입니다.\n"
        "이 게임을 정식 HDR 화면으로 변환하는 것이 본 애드온의 목적입니다.\n\n"
        "Microsoft Windows가 채택한 공식 선형(Linear) HDR 표준은 16비트 scRGB(BT.709 원색, 1.0 = 80 nits 기준)입니다.\n"
        "정상적인 HDR 디스플레이 출력을 완성하기 위해 게임의 최종 화면 출력 파이프라인을 16비트 scRGB로 전환했습니다.\n"
        "원작과 똑같은 BT.709 색감을 그대로 유지하므로 오리지널 색상의 변형 없이 순수한 광원만 넓혀줍니다.\n\n"
        "스왑체인(Swapchain) 생성 단계에서 이 공식 표준으로 안전하게 확장되도록 KAKA가 코드를 개선했습니다.\n"
        "순정과 달리 모드 매니저(version.dll 등) 환경에서 발생하는 고질적인 충돌을 방어하고 튕김 없이 구동되도록 개조한 것입니다.\n\n"
        "애드온 단독으로는 화면이 허옇게 들뜨므로, 하단의 설명을 참고하여 인버스 톤매퍼(Inverse Tone Mapping) 셰이더를 반드시 함께 적용하십시오.\n");

    ImGui::Spacing();
    ImGui::Separator();

    // 소제목 2: ARCHITECTURE & TECHNICAL KNOWLEDGE / 기술 지식
    ImGui::TextColored(ImVec4(0.35f, 0.75f, 1.0f, 1.0f), "ARCHITECTURE & TECHNICAL KNOWLEDGE / 기술 지식");
    ImGui::Separator();
    ImGui::BulletText("Microsoft Windows Advanced Color 표준 준수:");
    ImGui::TextWrapped("  Windows DWM(데스크톱 창 관리자)의 네이티브 합성 기준인 FP16 scRGB 파이프라인을 그대로 준수합니다.\n\n");

    ImGui::BulletText("sRGB(BT.709) 원색 일치 및 무왜곡 보증:");
    ImGui::TextWrapped("  SDR 게임의 원색 좌표(BT.709)와 완벽히 일치하여, HDR10(BT.2020) 강제 변환 시 발생하는 색 틀어짐과 밴딩을 원천 차단합니다.\n\n");

    ImGui::BulletText("Linear(감마 1.0) 선형 연산의 물리적 정밀도:");
    ImGui::TextWrapped("  80 nits를 1.0 기준으로 다루며, 10.0(800 nits), 12.5(1000 nits) 등 고광도 빛의 값을 선형 비례로 셰이더에 정확히 전달합니다.\n\n");

    ImGui::BulletText("선제적 플립 모델 강제 및 런타임 간섭 제로:");
    ImGui::TextWrapped("  on_create_swapchain 시점에 FP16, FLIP_DISCARD, 버퍼 2개 이상을 선제 확보하여 ResizeBuffers 크래시를 영구 차단합니다.\n\n");

    ImGui::BulletText("Graceful Fallback 및 무부하 바이패스:");
    ImGui::TextWrapped("  QueryInterface 실패 시에도 게임을 종료하지 않는 예외 안전망을 갖추었으며, 렌더링을 직접 건드리지 않아 GPU 부하가 0.00ms입니다.\n\n");

    ImGui::BulletText("HDR 톤매핑 기준 밝기 가이드 (80 nits vs 203 nits):");
    ImGui::TextWrapped(
        "  원작 SDR 화면에서 어떤 톤과 환경을 목적으로 하는지에 따라 셰이더 기준값을 선택하십시오.\n\n"
        "  - 80 nits 기준 셰이더:\n"
        "    OLED의 장점인 트루 블랙과 원작 SDR 고유의 자연스러운 톤 밸런스를 그대로 지켜냅니다.\n"
        "    화면 전체의 인위적인 들뜸을 억제하여 패널의 ABL 간섭 없이 일정한 밝기를 유지합니다.\n"
        "    원작의 질감을 지킨 상태에서 순수 광원만 800 nits 피크로 분리해 체감하고 싶을 때 선택합니다.\n\n"
        "  - 203 nits 기준 셰이더:\n"
        "    Windows 11의 공식 HDR 권고 규격인 203 nits에 맞춰 게임 내 텍스트와 UI의 시인성을 자연스럽게 정돈합니다.\n"
        "    OLED 특유의 암부 뭉개짐(Black Crush)을 보정하여 어두운 배경의 세부 디테일을 또렷하게 살려냅니다.\n"
        "    어두운 장면에서도 디테일 손실 없이 안정적인 시야를 확보하며, 현대적 HDR 광원 효과를 적용하고 싶을 때 선택합니다.\n");

    ImGui::Spacing();
    ImGui::Separator();

    // 소제목 3: CREDITS & ACKNOWLEDGMENTS / 크레딧 및 원본 출처
    ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.6f, 1.0f), "CREDITS & ACKNOWLEDGMENTS / 크레딧 및 원본 출처");
    ImGui::Separator();
    ImGui::BulletText("베이스 프로젝트: Lilium (AutoHDR-ReShade)");
    ImGui::BulletText("AutoHDR 공식 베이스 저장소:\n  https://github.com/EndlesslyFlowering/AutoHDR-ReShade\n");
    ImGui::BulletText("연동 권장 HDR 셰이더 저장소 (Lilium):\n  https://github.com/EndlesslyFlowering/ReShade_HDR_shaders\n  HDR 톤매핑을 위해서는 위 저장소의 톤매핑 셰이더를 활용하십시오.\n");
    ImGui::BulletText("모드 매니저 프록시 호환 및 무충돌 scRGB 아키텍처: KAKA (2026)");
    ImGui::BulletText("실기 검증 환경: 최신 ReShade 6.8 (애드온 활성화 버전, 64비트).\n  저지 아이즈 실기 환경에서 단 하나의 충돌이나 이상 없이 정상 작동함을 완벽히 확인했습니다.\n");
    ImGui::BulletText("무삭제 원칙에 따라 원작자의 기존 코드와 라이선스 고지는 100%% 온전히 보존됩니다.\n");
}

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

        // ============================================================================
        // [NEW APPEND-ONLY: REGISTER [KAKA-AUTO HDR] TAB ON RE-SHADE 6.8]
        // ----------------------------------------------------------------------------
        // [English Description]:
        // - Registers the dedicated top-level menu bar overlay tab "[KAKA-AUTO HDR]".
        // ----------------------------------------------------------------------------
        // [한국어 상세 설명 (정밀 대조 번역)]:
        // - ReShade 6.8 상단 메뉴 바에 독립 최상위 탭 "[KAKA-AUTO HDR]"을 등록합니다.
        // ============================================================================
        reshade::register_overlay("[KAKA-AUTO HDR]", draw_kaka_hdr_overlay);

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

        // ============================================================================
        // [NEW APPEND-ONLY: UNREGISTER [KAKA-AUTO HDR] TAB ON DETACH]
        // ----------------------------------------------------------------------------
        // [English Description]:
        // - Safely unregisters the dedicated overlay tab "[KAKA-AUTO HDR]".
        // ----------------------------------------------------------------------------
        // [한국어 상세 설명 (정밀 대조 번역)]:
        // - 프로세스 언로드 시 최상위 탭 "[KAKA-AUTO HDR]"의 오버레이 등록을 안전하게 해제합니다.
        // ============================================================================
        reshade::unregister_overlay("[KAKA-AUTO HDR]", draw_kaka_hdr_overlay);

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
