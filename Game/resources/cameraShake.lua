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