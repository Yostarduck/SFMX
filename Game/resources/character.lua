-- Simple Lua script to control a character
-- File: character.lua

local speed = 200

-- Projectile stress demo (exercises async asset loading + pooled spawning):
--   HOLD F to fire continuously. Each new projectile asks the AssetManager to load its
--   texture ASYNCHRONOUSLY; the node + sprite are built inside the load callback, which
--   the engine fires from its main-thread pump once the texture finished decoding on a
--   worker (first shot decodes on the worker; the rest hit the cache).
--
--   Sustainability = a RECYCLE RING: the ring grows up to kMaxBullets once (bounded by
--   the fixed SpriteComponent pool), then, instead of allocating more, the OLDEST bullet
--   is repositioned and re-aimed. So holding F never exhausts the pools and never stops,
--   and in steady state there is zero C++ heap churn (no createChild / new sprite) —
--   only transform moves. This mirrors the engine's "reuse over destroy" rule.
local bullets      = {}     -- live ring: { node, vx, vy }
local kMaxBullets  = 900    -- headroom under the SpriteComponent pool (1024)
local kBulletSpeed = 350
local kFireBurst   = 16     -- projectiles launched per frame while F is held
local nextSlot     = 1      -- ring cursor for recycling the oldest bullet
local spawnAngle   = 0      -- rotating spray direction

-- Script driven by a ScriptComponent. The component runs the function returned below,
-- passing the owning SceneNode as `self` and the frame delta as `deltaTime`.
function update(self, deltaTime)
  local movement = Vector2f(0, 0)

  local wKey = Keyboard:isPressed(keyFromString("W"))
  local sKey = Keyboard:isPressed(keyFromString("S"))
  local aKey = Keyboard:isPressed(keyFromString("A"))
  local dKey = Keyboard:isPressed(keyFromString("D"))

  local lShiftKey = Keyboard:isPressed(keyFromString("LShift"))

  -- Held, not edge-triggered: fire for as long as F is down.
  local firing = Keyboard:isPressed(keyFromString("F"))

  movement.x = (dKey and 1 or 0) - (aKey and 1 or 0)
  movement.y = (sKey and 1 or 0) - (wKey and 1 or 0)

  local length = movement:length()
  if length > 0.0 then
    movement = movement:normalized()

    movement = movement * speed * deltaTime * (lShiftKey and 1 or 0.5)

    local myTransform = self:transform()
    myTransform:move(movement)
  end

  -- Advance every live projectile in its parent's frame.
  for i = 1, #bullets do
    local b = bullets[i]
    if b then
      b.node:transform():move(Vector2f(b.vx * deltaTime, b.vy * deltaTime))
    end
  end

  if firing then
    -- Snapshot the spawn origin now; spawn under the character's parent so the bullets
    -- fly independently of the character's own transform.
    local origin = self:transform():getPosition()
    local ox, oy = origin.x, origin.y
    local parent = self:getParent()
    local host = parent or self

    for k = 1, kFireBurst do
      spawnAngle = spawnAngle + 0.35           -- rotate the spray each shot
      local angle = spawnAngle
      local vx = math.cos(angle) * kBulletSpeed
      local vy = math.sin(angle) * kBulletSpeed

      if #bullets < kMaxBullets then
        -- Grow the ring: async-load the texture, build the node when it is ready.
        Assets.loadAsync("particle.png", function(tex)
          if not tex or not tex:isLoaded() then return end
          if #bullets >= kMaxBullets then return end   -- filled up while decoding

          local node = host:createChild("bullet")
          if not node then return end                  -- SceneNode pool full

          local sprite = node:addComponent(SpriteComponent)
          if sprite then
            sprite:setTextureAsset(tex)
            sprite:setScale(0.5)
          end
          node:transform():setPosition(Vector2f(ox, oy))
          bullets[#bullets + 1] = { node = node, vx = vx, vy = vy }
        end)
      else
        -- Ring full: recycle the oldest bullet. No new node/sprite, no async, no heap —
        -- just reposition and re-aim the existing one.
        local b = bullets[nextSlot]
        if b then
          b.node:transform():setPosition(Vector2f(ox, oy))
          b.vx = vx
          b.vy = vy
          nextSlot = nextSlot + 1
          if nextSlot > #bullets then nextSlot = 1 end
        end
      end
    end
  end
end

return update
