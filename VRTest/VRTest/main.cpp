/************************************************************************************
Filename    :   Win32_RoomTiny_Main.cpp
Content     :   First-person view test application for Oculus Rift
Created     :   11th May 2015
Authors     :   Tom Heath
Copyright   :   Copyright 2015 Oculus, Inc. All Rights reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*************************************************************************************/
/// This is an entry-level sample, showing a minimal VR sample, 
/// in a simple environment.  Use WASD keys to move around, and cursor keys.
/// Dismiss the health and safety warning by tapping the headset, 
/// or pressing any key. 
/// It runs with DirectX11.

// directxによるウィンドウ作成
#include "Win32_DirectXAppUtil.h"

// oculus sdkのインクルード
#include "OVR_CAPI_D3D.h"


//------------------------------------------------------------
// ovrSwapTextureSetラッパークラスで、D3D11レンダリングに必要なレンダーターゲットビュー
struct OculusTexture
{
	ovrSession               Session;
	ovrTextureSwapChain      TextureChain;
	ovrTextureSwapChain      DepthTextureChain;
	std::vector<ID3D11RenderTargetView*> TexRtv;
	std::vector<ID3D11DepthStencilView*> TexDsv;

	OculusTexture() :
		Session(nullptr),
		TextureChain(nullptr),
		DepthTextureChain(nullptr)
	{
	}

	bool Init(ovrSession session, int sizeW, int sizeH, int sampleCount, bool createDepth)
	{
		Session = session;

		// 最初にカラーテクスチャスワップチェーンを作成する
		{
			ovrTextureSwapChainDesc desc = {};
			desc.Type = ovrTexture_2D;
			desc.ArraySize = 1;
			desc.Width = sizeW;
			desc.Height = sizeH;
			desc.MipLevels = 1;
			desc.SampleCount = sampleCount;
			desc.Format = OVR_FORMAT_R8G8B8A8_UNORM_SRGB;
			desc.MiscFlags = ovrTextureMisc_DX_Typeless | ovrTextureMisc_AutoGenerateMips;
			desc.BindFlags = ovrTextureBind_DX_RenderTarget;
			desc.StaticImage = ovrFalse;

			ovrResult result = ovr_CreateTextureSwapChainDX(session, DIRECTX.Device, &desc, &TextureChain);
			if (!OVR_SUCCESS(result))
				return false;

			int textureCount = 0;
			ovr_GetTextureSwapChainLength(Session, TextureChain, &textureCount);
			for (int i = 0; i < textureCount; ++i)
			{
				ID3D11Texture2D* tex = nullptr;
				ovr_GetTextureSwapChainBufferDX(Session, TextureChain, i, IID_PPV_ARGS(&tex));

				D3D11_RENDER_TARGET_VIEW_DESC rtvd = {};
				rtvd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
				rtvd.ViewDimension = (sampleCount > 1) ? D3D11_RTV_DIMENSION_TEXTURE2DMS
					: D3D11_RTV_DIMENSION_TEXTURE2D;
				ID3D11RenderTargetView* rtv;
				HRESULT hr = DIRECTX.Device->CreateRenderTargetView(tex, &rtvd, &rtv);
				VALIDATE((hr == ERROR_SUCCESS), "Error creating render target view");
				TexRtv.push_back(rtv);
				tex->Release();
			}
		}

		// 要求された場合、深度スワップチェーンを作成する
		if (createDepth)
		{
			ovrTextureSwapChainDesc desc = {};
			desc.Type = ovrTexture_2D;
			desc.ArraySize = 1;
			desc.Width = sizeW;
			desc.Height = sizeH;
			desc.MipLevels = 1;
			desc.SampleCount = sampleCount;
			desc.Format = OVR_FORMAT_D32_FLOAT;
			desc.MiscFlags = ovrTextureMisc_None;
			desc.BindFlags = ovrTextureBind_DX_DepthStencil;
			desc.StaticImage = ovrFalse;

			ovrResult result = ovr_CreateTextureSwapChainDX(session, DIRECTX.Device, &desc, &DepthTextureChain);
			if (!OVR_SUCCESS(result))
				return false;

			int textureCount = 0;
			ovr_GetTextureSwapChainLength(Session, DepthTextureChain, &textureCount);
			for (int i = 0; i < textureCount; ++i)
			{
				ID3D11Texture2D* tex = nullptr;
				ovr_GetTextureSwapChainBufferDX(Session, DepthTextureChain, i, IID_PPV_ARGS(&tex));

				D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
				dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
				dsvDesc.ViewDimension = (sampleCount > 1) ? D3D11_DSV_DIMENSION_TEXTURE2DMS
					: D3D11_DSV_DIMENSION_TEXTURE2D;
				dsvDesc.Texture2D.MipSlice = 0;

				ID3D11DepthStencilView* dsv;
				HRESULT hr = DIRECTX.Device->CreateDepthStencilView(tex, &dsvDesc, &dsv);
				VALIDATE((hr == ERROR_SUCCESS), "Error creating depth stencil view");
				TexDsv.push_back(dsv);
				tex->Release();
			}
		}

		return true;
	}

	~OculusTexture()
	{
		for (int i = 0; i < (int)TexRtv.size(); ++i)
		{
			Release(TexRtv[i]);
		}
		for (int i = 0; i < (int)TexDsv.size(); ++i)
		{
			Release(TexDsv[i]);
		}
		if (TextureChain)
		{
			ovr_DestroyTextureSwapChain(Session, TextureChain);
		}
		if (DepthTextureChain)
		{
			ovr_DestroyTextureSwapChain(Session, DepthTextureChain);
		}
	}

	ID3D11RenderTargetView* GetRTV()
	{
		int index = 0;
		ovr_GetTextureSwapChainCurrentIndex(Session, TextureChain, &index);
		return TexRtv[index];
	}
	ID3D11DepthStencilView* GetDSV()
	{
		int index = 0;
		ovr_GetTextureSwapChainCurrentIndex(Session, DepthTextureChain, &index);
		return TexDsv[index];
	}

	// 変更をコミットする
	void Commit()
	{
		ovr_CommitTextureSwapChain(Session, TextureChain);
		ovr_CommitTextureSwapChain(Session, DepthTextureChain);
	}
};

// 後で再試行するためにtrueを返します（たとえば、表示が失われた後
static bool MainLoop(bool retryCreate)
{
	// これらをnullptrに初期化して、デバイスの失われた障害をきれいに処理する
	ovrMirrorTexture mirrorTexture = nullptr;
	OculusTexture  * pEyeRenderTexture[2] = { nullptr, nullptr };
	Scene          * roomScene = nullptr;
	Camera         * mainCam = nullptr;
	ovrMirrorTextureDesc mirrorDesc = {};
	long long frameIndex = 0;
	int msaaRate = 4;

	ovrSession session;
	ovrGraphicsLuid luid;
	ovrResult result = ovr_Create(&session, &luid);
	if (!OVR_SUCCESS(result))
		return retryCreate;

	ovrHmdDesc hmdDesc = ovr_GetHmdDesc(session);

	// デバイスとグラフィックのセットアップ
	// 注：ミラーウィンドウは任意のサイズにすることができます。このサンプルでは、​​HMD解像度の1/2を使用します
	if (!DIRECTX.InitDevice(hmdDesc.Resolution.w / 2, hmdDesc.Resolution.h / 2, reinterpret_cast<LUID*>(&luid)))
		goto Done;

	// 目をバッファに描画させます（実際のサイズ<HWの制限のために要求された場合は注意してください）。 
	ovrRecti         eyeRenderViewport[2];

	for (int eye = 0; eye < 2; ++eye)
	{
		ovrSizei idealSize = ovr_GetFovTextureSize(session, (ovrEyeType)eye, hmdDesc.DefaultEyeFov[eye], 1.0f);
		pEyeRenderTexture[eye] = new OculusTexture();
		if (!pEyeRenderTexture[eye]->Init(session, idealSize.w, idealSize.h, msaaRate, true))
		{
			if (retryCreate) goto Done;
			FATALERROR("Failed to create eye texture.");
		}
		eyeRenderViewport[eye].Pos.x = 0;
		eyeRenderViewport[eye].Pos.y = 0;
		eyeRenderViewport[eye].Size = idealSize;
		if (!pEyeRenderTexture[eye]->TextureChain || !pEyeRenderTexture[eye]->DepthTextureChain)
		{
			if (retryCreate) goto Done;
			FATALERROR("Failed to create texture.");
		}
	}

	// モニター上に表示するミラーを作成します。
	mirrorDesc.Format = OVR_FORMAT_R8G8B8A8_UNORM_SRGB;
	mirrorDesc.Width = DIRECTX.WinSizeW;
	mirrorDesc.Height = DIRECTX.WinSizeH;
	mirrorDesc.MirrorOptions = ovrMirrorOption_Default;
	result = ovr_CreateMirrorTextureWithOptionsDX(session, DIRECTX.Device, &mirrorDesc, &mirrorTexture);

	if (!OVR_SUCCESS(result))
	{
		if (retryCreate) goto Done;
		FATALERROR("Failed to create mirror texture.");
	}

	// ルームモデルを作成する
	roomScene = new Scene(false);

	// カメラを作成する
	mainCam = new Camera(XMVectorSet(0.0f, 0.0f, 5.0f, 0), XMQuaternionIdentity());

	// FloorLevelは、床の高さが0の場所を追跡する
	ovr_SetTrackingOriginType(session, ovrTrackingOrigin_FloorLevel);

	// メインループ
	while (DIRECTX.HandleMessages())
	{
		ovrSessionStatus sessionStatus;
		ovr_GetSessionStatus(session, &sessionStatus);
		if (sessionStatus.ShouldQuit)
		{
			// アプリケーションは終了を要求されているため、再試行を要求しないでください
			retryCreate = false;
			break;
		}
		if (sessionStatus.ShouldRecenter)
			ovr_RecenterTrackingOrigin(session);

		if (sessionStatus.IsVisible)
		{
			XMVECTOR forward = XMVector3Rotate(XMVectorSet(0, 0, -0.05f, 0), mainCam->Rot);
			XMVECTOR right = XMVector3Rotate(XMVectorSet(0.05f, 0, 0, 0), mainCam->Rot);
			if (DIRECTX.Key['W'] || DIRECTX.Key[VK_UP])      mainCam->Pos = XMVectorAdd(mainCam->Pos, forward);
			if (DIRECTX.Key['S'] || DIRECTX.Key[VK_DOWN])    mainCam->Pos = XMVectorSubtract(mainCam->Pos, forward);
			if (DIRECTX.Key['D'])                            mainCam->Pos = XMVectorAdd(mainCam->Pos, right);
			if (DIRECTX.Key['A'])                            mainCam->Pos = XMVectorSubtract(mainCam->Pos, right);
			static float Yaw = 0;
			if (DIRECTX.Key[VK_LEFT])  mainCam->Rot = XMQuaternionRotationRollPitchYaw(0, Yaw += 0.02f, 0);
			if (DIRECTX.Key[VK_RIGHT]) mainCam->Rot = XMQuaternionRotationRollPitchYaw(0, Yaw -= 0.02f, 0);

			// キューブをアニメートする
			static float cubePositionClock = 0;
			if (sessionStatus.HasInputFocus) // 入力がないと思われる場合は、アプリケーションを一時停止します。
				roomScene->Models[0]->Pos = XMFLOAT3(9 * sin(cubePositionClock), 3, 9 * cos(cubePositionClock += 0.015f));

			// 実行時に返される値（HmdToEyePoseなど）が変更される可能性があるため、ovrEyeRenderDescを取得するには、各フレームをovr_GetRenderDescで呼び出します。
			ovrEyeRenderDesc eyeRenderDesc[2];
			eyeRenderDesc[0] = ovr_GetRenderDesc(session, ovrEye_Left, hmdDesc.DefaultEyeFov[0]);
			eyeRenderDesc[1] = ovr_GetRenderDesc(session, ovrEye_Right, hmdDesc.DefaultEyeFov[1]);

			// 両方のアイポーズを同時に取得し、IPDオフセットは既に含まれています。
			ovrPosef EyeRenderPose[2];
			ovrPosef HmdToEyePose[2] = { eyeRenderDesc[0].HmdToEyePose,
										 eyeRenderDesc[1].HmdToEyePose };

			double sensorSampleTime;    // sensorSampleTimeは後でレイヤーに供給される
			ovr_GetEyePoses(session, frameIndex, ovrTrue, HmdToEyePose, EyeRenderPose, &sensorSampleTime);

			ovrTimewarpProjectionDesc posTimewarpProjectionDesc = {};

			// シーンをアイバッファにレンダリングする
			for (int eye = 0; eye < 2; ++eye)
			{
				// レンダリングターゲットを削除して設定する
				DIRECTX.SetAndClearRenderTarget(pEyeRenderTexture[eye]->GetRTV(), pEyeRenderTexture[eye]->GetDSV());
				DIRECTX.SetViewport((float)eyeRenderViewport[eye].Pos.x, (float)eyeRenderViewport[eye].Pos.y,
					(float)eyeRenderViewport[eye].Size.w, (float)eyeRenderViewport[eye].Size.h);

				// ポーズ情報をXM形式で取得する
				XMVECTOR eyeQuat = XMVectorSet(EyeRenderPose[eye].Orientation.x, EyeRenderPose[eye].Orientation.y,
					EyeRenderPose[eye].Orientation.z, EyeRenderPose[eye].Orientation.w);
				XMVECTOR eyePos = XMVectorSet(EyeRenderPose[eye].Position.x, EyeRenderPose[eye].Position.y, EyeRenderPose[eye].Position.z, 0);

				// リフトカメラのビュー行列と投影行列を取得する
				XMVECTOR CombinedPos = XMVectorAdd(mainCam->Pos, XMVector3Rotate(eyePos, mainCam->Rot));
				Camera finalCam(CombinedPos, XMQuaternionMultiply(eyeQuat, mainCam->Rot));
				XMMATRIX view = finalCam.GetViewMatrix();
				ovrMatrix4f p = ovrMatrix4f_Projection(eyeRenderDesc[eye].Fov, 0.2f, 1000.0f, ovrProjection_None);
				posTimewarpProjectionDesc = ovrTimewarpProjectionDesc_FromProjection(p, ovrProjection_None);
				XMMATRIX proj = XMMatrixSet(p.M[0][0], p.M[1][0], p.M[2][0], p.M[3][0],
					p.M[0][1], p.M[1][1], p.M[2][1], p.M[3][1],
					p.M[0][2], p.M[1][2], p.M[2][2], p.M[3][2],
					p.M[0][3], p.M[1][3], p.M[2][3], p.M[3][3]);
				XMMATRIX prod = XMMatrixMultiply(view, proj);
				roomScene->Render(&prod, 1, 1, 1, 1, true);

				// スワップチェーンへのコミットの取得
				pEyeRenderTexture[eye]->Commit();
			}

			// 単一のフルスクリーンFovレイヤーを初期化します。
			ovrLayerEyeFovDepth ld = {};
			ld.Header.Type = ovrLayerType_EyeFovDepth;
			ld.Header.Flags = 0;
			ld.ProjectionDesc = posTimewarpProjectionDesc;

			for (int eye = 0; eye < 2; ++eye)
			{
				ld.ColorTexture[eye] = pEyeRenderTexture[eye]->TextureChain;
				ld.DepthTexture[eye] = pEyeRenderTexture[eye]->DepthTextureChain;
				ld.Viewport[eye] = eyeRenderViewport[eye];
				ld.Fov[eye] = hmdDesc.DefaultEyeFov[eye];
				ld.RenderPose[eye] = EyeRenderPose[eye];
				ld.SensorSampleTime = sensorSampleTime;
			}

			ovrLayerHeader* layers = &ld.Header;
			result = ovr_SubmitFrame(session, frameIndex, nullptr, &layers, 1);
			// submitがエラーを返した場合はレンダリングループを終了し、ovrError_DisplayLostを再試行します。
			if (!OVR_SUCCESS(result))
				goto Done;

			frameIndex++;
		}

		// レンダーミラー
		ID3D11Texture2D* tex = nullptr;
		ovr_GetMirrorTextureBufferDX(session, mirrorTexture, IID_PPV_ARGS(&tex));

		DIRECTX.Context->CopyResource(DIRECTX.BackBuffer, tex);
		tex->Release();
		DIRECTX.SwapChain->Present(0, 0);

	}

	// 解放
Done:
	delete mainCam;
	delete roomScene;
	if (mirrorTexture)
		ovr_DestroyMirrorTexture(session, mirrorTexture);
	for (int eye = 0; eye < 2; ++eye)
	{
		delete pEyeRenderTexture[eye];
	}
	DIRECTX.ReleaseDevice();
	ovr_Destroy(session);

	// ovrError_DisplayLostで再試行
	return retryCreate || (result == ovrError_DisplayLost);
}

//-------------------------------------------------------------------------------------
int WINAPI WinMain(HINSTANCE hinst, HINSTANCE, LPSTR, int)
{
	// LibOVRとRiftを初期化する
	ovrInitParams initParams = { ovrInit_RequestVersion | ovrInit_FocusAware, OVR_MINOR_VERSION, NULL, 0, 0 };
	ovrResult result = ovr_Initialize(&initParams);
	VALIDATE(OVR_SUCCESS(result), "Failed to initialize libOVR.");

	VALIDATE(DIRECTX.InitWindow(hinst, L"VRTest"), "Failed to open window.");

	DIRECTX.Run(MainLoop);

	ovr_Shutdown();
	return(0);
}
