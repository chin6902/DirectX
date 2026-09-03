/*============================================================================
Contents   :  [camera.h]

Author     : Chin Qing You
LastUpdate : 2026/07/21
-----------------------------------------------------------------------------
World-space camera. Follows the player; converts world coords to screen.
============================================================================*/
#ifndef CAMERA_H
#define CAMERA_H

void Camera_Initialize();
void Camera_Update(float delta_time);

// Top-left of the visible world region
float Camera_GetX();
float Camera_GetY();

// world -> screen conversion 
float Camera_WorldToScreenX(float world_x);
float Camera_WorldToScreenY(float world_y);

float Camera_ScreenToWorldX(float screen_x);
float Camera_ScreenToWorldY(float screen_y);

void Camera_Shake(float strength, float duration);

#endif