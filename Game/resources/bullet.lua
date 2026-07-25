-- File: bullet.lua
-- Simple Lua script to control a bullet
local Bullet = {}

function Bullet.onCreated(self)
  self.gameManager = nil

  self.speed = 10
  self.maxLifetime = 20.0
  self.damage = 1
end

function Bullet.onStart(self)
  self.lifetime = 0.0
  
  local rotation = self.transform:getRotation()
  self.velocity = Vector2f(self.speed, 0):rotatedBy(rotation)
end

function Bullet.onUpdate(self, deltaTime)
  if self.gameManager.targetEnemy ~= nil then
    diff = self.gameManager.targetEnemy.transform:getPosition() - self.transform:getPosition()
    len = diff:lengthSquared()
    
    if len < 1000 then
      SceneManager:getActiveScene():destroyNode(self.owner)
      
      self.gameManager.targetEnemy.script:damage(self.damage)
      return
    end

    curAngle = self.velocity:angle()
    tarAngle = diff:angle()

    angle = tarAngle - curAngle

    self.velocity = self.velocity:rotatedBy(angle * deltaTime)
  end

  self.transform:move(self.velocity * deltaTime)

  self.lifetime = self.lifetime + deltaTime
  if self.lifetime >= self.maxLifetime then
    SceneManager:getActiveScene():destroyNode(self.owner)
  end
end

function Bullet.onDestroyed(self)
end

return Bullet