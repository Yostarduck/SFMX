-- File: enemy.lua
-- Lua script for enemy logic
local Enemy = {}

function Enemy.onCreated(self)
  self.gameManager = nil
  
  self.life = 10
  self.speed = 10
end

function Enemy.onStart(self)
end

function Enemy.onUpdate(self, deltaTime)
  self.transform:move(-self.speed * deltaTime, 0)

  if self.transform:getPosition().x < -640 then
    SceneManager:getActiveScene():destroyNode(self.owner)
  end
end

function Enemy.onDestroyed(self)
end

function Enemy.damage(self, amount)
  self.life = self.life - amount

  if self.life <= 0 then
    SceneManager:getActiveScene():destroyNode(self.owner)
    
    if self.gameManager ~= nil then
      self.gameManager:enemyDestroyed(self)
    end
  end
end

return Enemy