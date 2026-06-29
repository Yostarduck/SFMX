-- Simple Lua script to control a bullet
-- File: bullet.lua

local speed = 2000

local lifetime = 0.0
local maxLifetime = 1.0

-- Script driven by a ScriptComponent.
--
-- A script must return a table of optional lifecycle hooks, as below. Each hook
-- receives the owning SceneNode as `self`; onUpdate also receives the frame
-- delta.
--
--   onCreated(self)            -- once, after the component is linked to the node
--   onStart(self)              -- once, just before the first onUpdate
--   onUpdate(self, deltaTime)  -- every frame
--   onDestroyed(self)          -- once, when the component is destroyed
--
-- Two nodes can share this same file but each receives its own owner.
local Bullet = {}

function Bullet.onCreated(self)
  -- The node is fully linked here, so owner queries like getName() are valid.
  print("[bullet] created on node: " .. self:getName())
end

function Bullet.onStart(self)
  lifetime = 0.0
end

function Bullet.onUpdate(self, deltaTime)
  local myTransform = self:transform()
  local rotation = myTransform:getRotation()
  local movement = Vector2f(1, 0):rotatedBy(rotation)
  myTransform:move(movement * speed * deltaTime)

  lifetime = lifetime + deltaTime
  if lifetime >= maxLifetime then
    SceneManager:getActiveScene():destroyNode(self)
  end
end

function Bullet.onDestroyed(self)
  print("[bullet] destroyed node: " .. self:getName())
end

return Bullet
