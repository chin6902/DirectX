/*============================================================================
Contents   :  [flow_field.h]

Author     : Chin Qing You
LastUpdate : 2026/08/21
-----------------------------------------------------------------------------

============================================================================*/
#ifndef FLOW_FIELD_H
#define FLOW_FIELD_H

#include "vector2.h"

void FlowField_Initialize();
void FlowField_Finalize();

void FlowField_Update(const Vector2& target_world);

Vector2 FlowField_GetDirection(const Vector2& world_pos);

bool FlowField_HasRoute(const Vector2& world_pos);

void FlowField_DebugDraw();

#endif