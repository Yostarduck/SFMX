-- Simple Lua script to control a character
-- File: character.lua

local speed = 500.0

local scene
local cameraComponent
local myTransform
local currentPosition = Vector2f(0, 0)

-- Script driven by a ScriptComponent.
--
-- The script returns a table of lifecycle hooks (see bullet.lua for the full
-- set). `self` is the owning SceneNode, a C++ object exposed to Lua, so we can
-- call methods straight on it; `deltaTime` is the frame delta.
-- Two nodes can share this same file but each receives its own owner.
local Character = {}

function fireBullet(angle)
  local bullet = scene:createNode("Bullet")
  bullet:transform():setPosition(currentPosition)
  bullet:transform():setRotation(angle)

  local assetID = UUID.createFromName("particle.png")

  local bulletSprite = bullet:addComponent(SpriteComponent)
  bulletSprite:setTextureAssetId(assetID)
  local spriteSize = bulletSprite:getPixelSize()
  local spriteOrigin = Vector2f(spriteSize.x, spriteSize.y) * 0.5
  bulletSprite:setOrigin(spriteOrigin)
  bulletSprite:setScale(0.5)

  local scriptID = UUID.createFromName("bullet.lua")
  local bulletScript = bullet:addComponent(ScriptComponent, scriptID)
end

function Character.onCreated(self)
  myTransform = self:transform()
  scene = SceneManager:getActiveScene()
  cameraComponent = scene:getCamera()
end

function Character.onStart(self)
  if scene == nil then
    scene = SceneManager:getActiveScene()
  end

  buttonNode = scene:findNode("StartBtn")
  if buttonNode == nil then
    print("Start button node not found!")
    return
  end

  uiButtonComponent = buttonNode:getComponent(UIButton)
  if uiButtonComponent == nil then
    print("UIButtonComponent not found on Start button node!")
    return
  end
  
  uiButtonComponent:onSubmit(self, "customAction")

  print("Character script started.")
end

function Character.onUpdate(self, deltaTime)  
  if scene == nil then
    scene = SceneManager:getActiveScene()
  end
  if cameraComponent == nil and scene ~= nil then
    cameraComponent = scene:getCamera()
  end

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

  currentPosition = myTransform:getPosition()

  if shoot then
    local target = cameraComponent:screenToWorld(Mouse:getPosition(), Vector2i(1024, 768))

    local direction = Vector2f(target.x, target.y) - currentPosition
    local angle = direction:normalized():angle()

    fireBullet(angle)
  end

end

function Character.customAction(self)
  print("Custom action triggered!")
end

return Character