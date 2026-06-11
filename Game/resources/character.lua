-- Simple Lua script to control a character
-- File: character.lua

local wKey, sKey, aKey, dKey
local speed = 200

function update(deltaTime)
    wKey = keyPressed("W")
    sKey = keyPressed("S")
    aKey = keyPressed("A")
    dKey = keyPressed("D")
    
    direction = { x = 0, y = 0 }
    direction.x = (dKey and 1 or 0) - (aKey and 1 or 0)
    direction.y = (wKey and 1 or 0) - (sKey and 1 or 0)

    -- Normalize direction
    local length = math.sqrt(direction.x * direction.x + direction.y * direction.y)
    if length > 0 then
        direction.x = direction.x / length
        direction.y = direction.y / length
    end

   direction.x = direction.x * speed * deltaTime
   direction.y = direction.y * speed * deltaTime 

   print("Direction: (" .. direction.x .. ", " .. direction.y .. ")")
end