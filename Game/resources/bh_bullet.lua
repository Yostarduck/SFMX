-- File: bh_bullet.lua
-- Bullet script for the bullet hell game

local BHBullet = {}

function BHBullet.onCreated(self)
  self.gameManager = nil

  self.speed = 10
  self.maxLifetime = 20.0
  self.damage = 1
  self.rotationSpeed = 0
  self.direction = Vector2f(0,0)
end

function BHBullet.onStart(self)
  self.lifetime = 0.0

  local rotation = self.transform.getRotation()
  self.direction = Vector2f(0, -1):rotatedBy(rotation)
end

function BHBullet.onUpdate(self, deltaTime)
  self.transform:move(self.direction * self.speed * deltaTime)
  self.transform:rotate(self.rotationSpeed * deltaTime)
  self.lifetime = self.lifetime + deltaTime;
  if self.lifetime >= self.maxLifetime then
    SceneManager:getActiveScene():destroyNode(self.owner)
  end
end

