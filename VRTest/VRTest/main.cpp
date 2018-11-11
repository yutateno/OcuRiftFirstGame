/************************************************************************************
Filename    :   Win32_RoomTiny_Main.cpp
Content     :   First-person view test application for Oculus Rift
Created     :   18th Dec 2014
Authors     :   Tom Heath
Copyright   :   Copyright 2012 Oculus, Inc. All Rights reserved.

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
/// This is the same sample as OculusRoomTiny(DX11), 
/// but this time using the standard set of 
/// utility functions provided in BasicVR.h - these functions we will be
/// used in subsequent samples.

#include "../Common/Win32_DirectXAppUtil.h" // DirectX
#include "../Common/Win32_BasicVR.h"        // Basic VR
#include "../Common/Win32_CameraCone.h" // Camera cone

struct UsingBasicVR : BasicVR
{
	UsingBasicVR(HINSTANCE hinst) : BasicVR(hinst, L"VRTest") {}

	void MainLoop()
	{
		Layer[0] = new VRLayer(Session);

		CameraCone cameraCone(this);


		// Create a trivial model to represent the left controller
		TriangleSet cube;
		cube.AddSolidColorBox(0.05f, -0.05f, 0.05f, -0.05f, 0.05f, -0.05f, 0xff404040);
		Model * controllerL = new Model(&cube, XMFLOAT3(0, 0, 0), XMFLOAT4(0, 0, 0, 1), new Material(new Texture(false, 256, 256, Texture::AUTO_CEILING)));
		Model * controllerR = new Model(&cube, XMFLOAT3(0, 0, 0), XMFLOAT4(0, 0, 0, 1), new Material(new Texture(false, 256, 256, Texture::AUTO_CEILING)));

		TriangleSet testCube;
		testCube.AddSolidColorBox(1.4f, 1.1f, -20.0f, 0.1f, 0.0f, -20.1f, 0xff505050); // Right Bars)
		Model* testModel=new Model(&testCube, XMFLOAT3(0, 0, 0), XMFLOAT4(0, 0, 0, 1), new Material(new Texture(false, 256, 256, Texture::AUTO_CEILING)));

		TriangleSet testBlock;
		XMFLOAT3 blockArea(1.8f, 1.0f, 1.8f);
		testBlock.AddSolidColorBox(0.05f, -0.05f, 0.05f, -0.05f, 0.05f, -0.05f, 0xff404040); // Right Bars)
		Model* testBlockModel = new Model(&testBlock, blockArea, XMFLOAT4(0, 0, 0, 1), new Material(new Texture(false, 256, 256, Texture::AUTO_CEILING)));

		float red = 0.0f;

		XMFLOAT3 preRightPos;
		XMFLOAT4 preRightRot;

		while (HandleMessages())
		{
			// We don't allow yaw change for now, as this sample is too simple to cater for it.
			ActionFromInput(0.0f, false, true);
			ovrTrackingState hmdState = Layer[0]->GetEyePoses();
			ovrTrackerPose   trackerPose = ovr_GetTrackerPose(Session, 0);

			//Write position and orientation into controller models.
			controllerL->Pos = XMFLOAT3(XMVectorGetX(MainCam->Pos) + hmdState.HandPoses[ovrHand_Left].ThePose.Position.x,
				XMVectorGetY(MainCam->Pos) + hmdState.HandPoses[ovrHand_Left].ThePose.Position.y,
				XMVectorGetZ(MainCam->Pos) + hmdState.HandPoses[ovrHand_Left].ThePose.Position.z);
			controllerL->Rot = XMFLOAT4(hmdState.HandPoses[ovrHand_Left].ThePose.Orientation.x,
				hmdState.HandPoses[ovrHand_Left].ThePose.Orientation.y,
				hmdState.HandPoses[ovrHand_Left].ThePose.Orientation.z,
				hmdState.HandPoses[ovrHand_Left].ThePose.Orientation.w);

			controllerR->Pos = XMFLOAT3(XMVectorGetX(MainCam->Pos) + hmdState.HandPoses[ovrHand_Right].ThePose.Position.x,
				XMVectorGetY(MainCam->Pos) + hmdState.HandPoses[ovrHand_Right].ThePose.Position.y,
				XMVectorGetZ(MainCam->Pos) + hmdState.HandPoses[ovrHand_Right].ThePose.Position.z);
			controllerR->Rot = XMFLOAT4(hmdState.HandPoses[ovrHand_Right].ThePose.Orientation.x,
				hmdState.HandPoses[ovrHand_Right].ThePose.Orientation.y,
				hmdState.HandPoses[ovrHand_Right].ThePose.Orientation.z,
				hmdState.HandPoses[ovrHand_Right].ThePose.Orientation.w);

			//Button presses are modifying the colour of the controller model below
			ovrInputState inputState;
			ovr_GetInputState(Session, ovrControllerType_Touch, &inputState);
			// ovrControllerType touchController[] = { ovrControllerType_LTouch, ovrControllerType_RTouch };

			///		this->MainCam->Pos = initialPos = controllerL->Pos;

			// Some auxiliary controls we're going to read from the remote. 
			XMVECTOR forward = XMVector3Rotate(XMVectorSet(0, 0, -0.05f, 0), MainCam->Rot);
			XMVECTOR right = XMVector3Rotate(XMVectorSet(0.05f, 0, 0, 0), MainCam->Rot);

			if (inputState.Thumbstick[ovrHand_Left].y > 0.5f) MainCam->Pos = XMVectorAdd(MainCam->Pos, forward);
			if (inputState.Thumbstick[ovrHand_Left].y < -0.5f) MainCam->Pos = XMVectorSubtract(MainCam->Pos, forward);
			if (inputState.Thumbstick[ovrHand_Left].x < -0.5f) MainCam->Pos = XMVectorSubtract(MainCam->Pos, right);
			if (inputState.Thumbstick[ovrHand_Left].x > 0.5f) MainCam->Pos = XMVectorAdd(MainCam->Pos, right);

			// 右手とブロックがほぼ重なったら
			if (controllerR->Pos.x > testBlockModel->Pos.x - 0.1f
				&& controllerR->Pos.x < testBlockModel->Pos.x + 0.1f
				&& controllerR->Pos.y > testBlockModel->Pos.y - 0.1f
				&& controllerR->Pos.y < testBlockModel->Pos.y + 0.1f
				&& controllerR->Pos.z > testBlockModel->Pos.z - 0.1f
				&& controllerR->Pos.z < testBlockModel->Pos.z + 0.1f)
			{
				// バイブレーションをさせる
				ovr_SetControllerVibration(Session, ovrControllerType_RTouch, 1.0f, inputState.HandTrigger[ovrHand_Right] * 1.025f);

				// トリガーを押したら
				if (inputState.HandTrigger[ovrHand_Right] > 0.5f)
				{
					red = 0.5f;
					testBlockModel->Pos.x += (controllerR->Pos.x - preRightPos.x);
					testBlockModel->Pos.y += (controllerR->Pos.y - preRightPos.y);
					testBlockModel->Pos.z += (controllerR->Pos.z - preRightPos.z);

					// 回転度数の限界を設定しないとダメ。
					//testBlockModel->Rot.x = (controllerR->Rot.x/* - preRightRot.x*/);
					//testBlockModel->Rot.y = (controllerR->Rot.y/* - preRightRot.y*/);
					//testBlockModel->Rot.z = (controllerR->Rot.z/* - preRightRot.z*/);
					//testBlockModel->Rot.w = (controllerR->Rot.w/* - preRightRot.w*/);
				}
				else red = 1.0f;
			}
			else red = 0.0f;


			// Xボタンでガーディアンの色を変更
			if ((inputState.Buttons & ovrButton_X) != 0)
			{
				ovrBoundaryLookAndFeel colorz;
				float rand1 = (float)rand() / RAND_MAX;
				float rand2 = (float)rand() / RAND_MAX;
				float rand3 = (float)rand() / RAND_MAX;
				colorz.Color = { rand1, rand2, rand3, 1.0f };
				ovr_SetBoundaryLookAndFeel(Session, &colorz);
			}
			
			for (int eye = 0; eye < 2; ++eye)
			{
				XMMATRIX viewProj = Layer[0]->RenderSceneToEyeBuffer(MainCam, RoomScene, eye);

				// コントローラーのモデルを描画
				controllerL->Render(&viewProj, 1, inputState.Buttons & ovrTouch_X ? 1.0f : 0.0f,
					inputState.Buttons & ovrTouch_Y ? 1.0f : 0.0f, 1, true);
				controllerR->Render(&viewProj, 1, inputState.Buttons & ovrTouch_A ? 1.0f : 0.0f,
					inputState.Buttons & ovrTouch_B ? 1.0f : 0.0f, 1, true);

				// テスト用のモデルを描画
				testModel->Render(&viewProj, 1, red, 0.0f, 1, true);
				testBlockModel->Render(&viewProj, 1, red, 0.0f, 1, true);
			}

			// 直前の右手の位置
			preRightPos = controllerR->Pos;
			preRightRot = controllerR->Rot;

			Layer[0]->PrepareLayerHeader();
			DistortAndPresent(1);
		}
		
		delete testBlockModel;
		delete testModel;
		delete controllerL;
		delete controllerR;
	}
};

//-------------------------------------------------------------------------------------
int WINAPI WinMain(HINSTANCE hinst, HINSTANCE, LPSTR, int)
{
	UsingBasicVR app(hinst);
	return app.Run();
}
