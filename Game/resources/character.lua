-- Simple Lua script to control a character
-- File: character.lua

local speed = 500

-- Script driven by a ScriptComponent.
--
-- The component runs the function returned below, passing the owning SceneNode
-- as `self` and the frame delta as `deltaTime`. `self` is a C++ object exposed
-- to Lua, so we can call methods straight on it.
-- Two nodes can share this same file but each receives its own owner.
function update(self, deltaTime)
  local myTransform = self:transform()
  
  local wKey = Keyboard:isPressed(keyFromString("W"))
  local sKey = Keyboard:isPressed(keyFromString("S"))
  local aKey = Keyboard:isPressed(keyFromString("A"))
  local dKey = Keyboard:isPressed(keyFromString("D"))
  local lShiftKey = Keyboard:isPressed(keyFromString("LShift"))

  local shoot = Mouse:wasPressedThisFrame(MouseButton.Left)

  local movement = Vector2f((dKey and 1 or 0) - (aKey and 1 or 0),
                            (sKey and 1 or 0) - (wKey and 1 or 0))

  local length = movement:length()
  if length > 0.0 then
    movement = movement:normalized()

    movement = movement * speed * deltaTime * (lShiftKey and 2.0 or 1.0)
    
    myTransform:move(movement)
  end

  if shoot then
    local scene = SceneManager:getActiveScene()

    local cameraComponent = scene:getCamera()

    local source = myTransform:getPosition()
    local target = cameraComponent:screenToWorld(Mouse:getPosition(), Vector2i(1024, 768))
    
    local direction = Vector2f(target.x, target.y) - source
    local angle = direction:normalized():angle()

    local bullet = scene:createNode("Bullet")
    bullet:transform():setPosition(source)
    bullet:transform():setRotation(angle)

    local assetID = UUID.createFromName("particle.png")

    local bulletSprite = bullet:addComponent(SpriteComponent)
    bulletSprite:setTextureAssetId(assetID)
    local spriteSize = bulletSprite:getPixelSize()
    local spriteOrigin = Vector2f(spriteSize.x, spriteSize.y) * 0.5
    bulletSprite:setOrigin(spriteOrigin)
    bulletSprite:setScale(0.2)

    local bulletScript = bullet:addComponent(ScriptComponent, "Game/resources/bullet.lua")
  end

end

return update