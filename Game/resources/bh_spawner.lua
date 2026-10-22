
local HBSpawner = {}

function HBSpawner.onCreated(self)
  self.entityOwner = nil
  self.gameManager = nil

  self.spawnInterval = 1.0
  self.BHBulletScriptID = UUID.createFromName("bh_bullet.lua")
  self.bulletAssetID = UUID.createFromName("particle.png")

end

function HBSpawner.onStart(self)
  self.timeSinceLastSpawn = 0.0
end

function HBSpawner.onUpdate(self, deltaTime)
  self.timeSinceLastSpawn = self.timeSinceLastSpawn + deltaTime

  if self.timeSinceLastSpawn >= self.spawnInterval then
    self.timeSinceLastSpawn = 0.0

    local scene = SceneManager:getActiveScene()
    local bulletNode = scene:createNode("BHBulletInstance")
    bulletNode.transform:setPosition(self.transform:getPosition())
    bulletNode.transform:setRotation(self.transform:getRotation())

    local bulletScript = bulletNode:addScript(self.BHBulletScriptID)
    bulletScript.gameManager = self.gameManager
  end
end
