-- Simple Lua script to control a character
-- File: character.lua

local speed = 200

-- Script driven by a ScriptComponent.
--
-- The component runs the function returned below, passing the owning SceneNode
-- as `self` and the frame delta as `deltaTime`. `self` is a C++ object exposed
-- to Lua, so we can call methods straight on it: here we read the owner's name
-- and position and print them. Two nodes share this same file but each receives
-- its own owner, so each prints its own position.
function update(self, deltaTime)
  local movement = Vector2f(0, 0)

  local wKey = Keyboard:isPressed(keyFromString("W"))
  local sKey = Keyboard:isPressed(keyFromString("S"))
  local aKey = Keyboard:isPressed(keyFromString("A"))
  local dKey = Keyboard:isPressed(keyFromString("D"))

  local lShiftKey = Keyboard:isPressed(keyFromString("LShift"))

  local shoot = Keyboard:wasPressedThisFrame(keyFromString("F"))

  movement.x = (dKey and 1 or 0) - (aKey and 1 or 0)
  movement.y = (sKey and 1 or 0) - (wKey and 1 or 0)

  local length = movement:length()
  if length > 0.0 then
    movement = movement:normalized()

    movement = movement * speed * deltaTime * (lShiftKey and 1 or 0.5)

    local myTransform = self:transform()
    myTransform:move(movement)
  end

  if shoot then
    print(self:getName() .. " shoots!")
  end

end

return update