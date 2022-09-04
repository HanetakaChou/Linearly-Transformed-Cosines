//
// Copyright (C) YuqiaoZhang(HanetakaChou)
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//

#ifndef _CAMERA_CONTROLLER_H_
#define _CAMERA_CONTROLLER_H_ 1

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif
#ifndef NOMINMAX
#define NOMINMAX 1
#endif
#include <sdkddkver.h>
#include <windows.h>
#include <DirectXMath.h>

class CameraController
{
	float Previous_X;
	float Previous_Y;

public:
	DirectX::XMFLOAT3 m_eye_position;
	DirectX::XMFLOAT3 m_eye_direction;
	DirectX::XMFLOAT3 m_up_direction;

	void MoveForward();
	void MoveBack();
	void MoveLeft();
	void MoveRight();
	void MoveUp();
	void MoveDown();

	void OnMouseMove(float x, float y, bool hold);
};

extern class CameraController g_camera_controller;

#endif