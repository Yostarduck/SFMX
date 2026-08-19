-- File: cameraShake.lua
-- Simple Lua script to control camera shake effect

local CameraShake = {}

function CameraShake.onCreated(self)
  self.shakeDuration = 0.5
  self.shakeMagnitude = 0.1
  self.shakeTimer = 0.0
  self.originalPosition = Vector2f(0, 0)
end

function CameraShake.onStart(self)
  self.shakeTimer = 0.0
  self.originalPosition = self.transform:getPosition()
end

function CameraShake.onUpdate(self, deltaTime)
  if self.shakeTimer > 0 then
    self.shakeTimer = self.shakeTimer - deltaTime

    local offsetX = (math.random() * 2 - 1) * self.shakeMagnitude
    local offsetY = (math.random() * 2 - 1) * self.shakeMagnitude

    self.transform:setPosition(self.originalPosition + Vector2f(offsetX, offsetY))
  else
    self.transform:setPosition(self.originalPosition)
  end
end

function CameraShake.startShake(self, duration, magnitude)
  self.shakeDuration = duration or self.shakeDuration
  self.shakeMagnitude = magnitude or self.shakeMagnitude
  self.shakeTimer = self.shakeDuration
  self.originalPosition = self.transform:getPosition()
end

return CameraShake
