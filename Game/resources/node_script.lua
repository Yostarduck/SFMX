local speed = 100

-- Script driven by a ScriptComponent.
--
-- The component runs the function returned below, passing the owning SceneNode
-- as `self` and the frame delta as `deltaTime`. `self` is a C++ object exposed
-- to Lua, so we can call methods straight on it: here we read the owner's name
-- and position and print them. Two nodes share this same file but each receives
-- its own owner, so each prints its own position.
return function(self, deltaTime)
  movement = Vector2f(0, 0)

  movement.x = (keyPressed("D") and 1 or 0) - (keyPressed("A") and 1 or 0)
  movement.y = (keyPressed("S") and 1 or 0) - (keyPressed("W") and 1 or 0)

  length = math.sqrt(movement.x * movement.x + movement.y * movement.y)
  if length > 0 then
    movement.x = movement.x / length
    movement.y = movement.y / length

    movement.x = movement.x * speed * deltaTime
    movement.y = movement.y * speed * deltaTime
  
    local myTransform = self:transform()
    myTransform:move(movement)
  end
end
