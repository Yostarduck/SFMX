-- Simple Lua script to control a bullet
-- File: bullet.lua

local speed = 100

local lifetime = 0.0
local maxLifetime = 1.0

-- Precomputed straight-line velocity (world units/sec), filled in onStart.
-- A bullet's rotation never changes, so we resolve the direction once instead
-- of allocating Vector2f/Angle userdata every frame.
local vx = 0.0
local vy = 0.0

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

--function Bullet.onCreated(self)
  -- The node is fully linked here, so owner queries like getName() are valid.
--end

function Bullet.onStart(self)
  lifetime = 0.0

  -- Resolve the constant velocity from the (fixed) spawn rotation just once.
  local rotation = self:transform():getRotation()
  local direction = Vector2f(1, 0):rotatedBy(rotation)
  vx = direction.x * speed
  vy = direction.y * speed
end

function Bullet.onUpdate(self, deltaTime)
  -- Hot path: only a scalar move, no Lua userdata allocations.
  self:transform():move(vx * deltaTime, vy * deltaTime)

  lifetime = lifetime + deltaTime
  if lifetime >= maxLifetime then
    SceneManager:getActiveScene():destroyNode(self)
  end
end

--function Bullet.onDestroyed(self)
--end

return Bullet
