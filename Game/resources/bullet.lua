-- Simple Lua script to control a bullet
-- File: bullet.lua

local speed = 2000

local lifetime = 0.0
local maxLifetime = 1.0

-- Script driven by a ScriptComponent.
--
-- The component runs the function returned below, passing the owning SceneNode
-- as `self` and the frame delta as `deltaTime`. `self` is a C++ object exposed
-- to Lua, so we can call methods straight on it.
-- Two nodes can share this same file but each receives its own owner.
function update(self, deltaTime)  
  local myTransform = self:transform()
  local rotation = myTransform:getRotation()
  local movement = Vector2f(1, 0):rotatedBy(rotation)
  myTransform:move(movement * speed * deltaTime)

  lifetime = lifetime + deltaTime
  if lifetime >= maxLifetime then
    SceneManager:getActiveScene():destroyNode(self)
  end
end

return update