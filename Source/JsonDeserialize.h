#pragma once

#include "rad/IO/JsonValue.h"
#include "rad/VulkanEngine/Core.h"
#include "glm/glm.hpp"

void FromJson(const rad::JsonValueRef& json, VulkanShaderMacro& macro);
