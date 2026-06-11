-- Script driven by a ScriptComponent.
--
-- The component runs the function returned below, passing the owning SceneNode
-- as `self` and the frame delta as `deltaTime`. `self` is a C++ object exposed
-- to Lua, so we can call methods straight on it: here we read the owner's name
-- and position and print them. Two nodes share this same file but each receives
-- its own owner, so each prints its own position.
return function(self, deltaTime)
  local myTransform = self:transform()
  local position = myTransform:getPosition()
  if keyPressed("LShift") then
    print("[Script] " .. self:getName() .. " is at (" .. position.x .. ", " .. position.y .. ")")
  end
end
