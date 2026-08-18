-- File: genericMage.lua
-- Lua script to handle a Mage behaviour
local GenericMage = {}

GenericMage.spriteUUID = nil

function GenericMage.onCreated(self)
  self.gameManager = nil

  self.bulletSpeed = 100
  self.bulletDamage = 1
  self.bulletShootAngle = Vector2f(20, 70)
  self.bulletCooldown = 3

  self.scene = SceneManager:getActiveScene()
  self.bulletSpawnPosition = self.transform:getPosition() + Vector2f(25, -45)

  self.bulletAssetID = UUID.createFromName("particle.png")
  self.bulletScriptID = UUID.createFromName("bullet.lua")
end

function GenericMage.onStart(self)
  mySprite = self.owner:addComponent(SpriteComponent)
  mySprite:setTextureAssetId(GenericMage.spriteUUID)
  spriteSize = mySprite:getPixelSize()
  spriteOrigin = Vector2f(spriteSize.x, spriteSize.y) * 0.5
  mySprite:setOrigin(spriteOrigin)
  mySprite:setScale(0.5)

  self.cooldown = self.bulletCooldown
end

function GenericMage.onUpdate(self, deltaTime)
  self.cooldown = self.cooldown - deltaTime

  if self.cooldown < 0 then
    fireBullet(self)

    self.cooldown = self.bulletCooldown
  end
end

function fireBullet(self)
  angle = math.random(270, 360)

  bullet = self.scene:createNode("Bullet")
  bullet:transform():setPosition(self.bulletSpawnPosition)
  bullet:transform():setRotation(Angle.degrees(angle))  

  bulletSprite = bullet:addComponent(SpriteComponent)
  bulletSprite:setTextureAssetId(self.bulletAssetID)
  spriteSize = bulletSprite:getPixelSize()
  spriteOrigin = Vector2f(spriteSize.x, spriteSize.y) * 0.5
  bulletSprite:setOrigin(spriteOrigin)
  bulletSprite:setScale(0.1)

  bulletScriptComponent = bullet:addComponent(ScriptComponent, self.bulletScriptID)

  bulletScript = bulletScriptComponent:instance()
  if bulletScript ~= nil then
    bulletScript.gameManager = self.gameManager
    bulletScript.speed = self.bulletSpeed
    bulletScript.damage = self.bulletDamage
  end
end

return GenericMage