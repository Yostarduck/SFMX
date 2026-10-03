-- file: gameManager.lua
-- Experimental Lua script to handle game logic
local GameManager = {}

-- Scene references
local scene
local camera

-- UI references
local infoLabel
local upgradesMenuButton
local upgradesMenuContainer

local buySlider
local buyCommonMageButton
local buyFireMageButton
local buyThunderMageButton
local buyElderWizardButton
local buyEliteWarlockButton

-- units
local Enemies       = {}
local CommonMages   = {}
local FireMages     = {}
local ThunderMages  = {}
local ElderWizards  = {}
local EliteWarlocks = {}

-- Units
local CommonMageData   = { cost = 1,    assetID = UUID.createFromName("CatDrooling.png"),  yOffset = 0,   damage = 1    }
local FireMageData     = { cost = 3,    assetID = UUID.createFromName("CatHappy.png"),     yOffset = 100, damage = 5    }
local ThunderMageData  = { cost = 10,   assetID = UUID.createFromName("CatPop.png"),       yOffset = 200, damage = 15   }
local ElderWizardData  = { cost = 25,   assetID = UUID.createFromName("CatTongue.png"),    yOffset = 300, damage = 30   }
local EliteWarlockData = { cost = 100,  assetID = UUID.createFromName("CatSoldier.png"),   yOffset = 400, damage = 150  }

-- Game Settings
local enemySpawnLocation = Vector2f(1280, 580)
local unitSpawnLocation = Vector2f(50, 580)
local money = 1

local startSpawnRate = 30
local enemySpawnRate = 0
local spawnCooldown = 0
local playerDamage = 0

local particles = nil
local particlesConfig = nil

function GameManager.onCreated(self)
  self.targetEnemy = nil
end

function GameManager.onStart(self)
  scene = SceneManager:getActiveScene()
  
  myScript = self.owner:getComponent(ScriptComponent)
  
  infoLabel = UI:get("InfoLabel")
  if infoLabel == nil then
    print("infoLabel not found")
  end

  upgradesMenuButton = UI:get("UpgradesButton")
  if upgradesMenuButton ~= nil then
    upgradesMenuButton:onPointerClick(myScript, "toggleMenu")
  else
    print("UpgradesButton not found")
  end

  upgradesMenuContainer = UI:get("UpgradesMenu")
  if upgradesMenuContainer ~= nil then
    upgradesMenuContainer:setEnabled(false)
    upgradesMenuContainer:setVisible(false)
  else
    print("UpgradesMenu not found")
  end

  buySlider = UI:get("BuySlider")
  if buySlider == nil then
    print("BuySlider not found")
  end

  commonMageCostLabel = UI:get("Common Mage Cost Label")
  if commonMageCostLabel ~= nil then
    commonMageCostLabel:setText("$" .. CommonMageData.cost)
  end

  buyCommonMageButton = UI:get("Common Mage Button")
  if buyCommonMageButton ~= nil then
    buyCommonMageButton:onPointerClick(myScript, "buyCommonMage")
  else
    print("Common Mage Button not found")
  end

  fireMageCostLabel = UI:get("Fire Mage Cost Label")
  if fireMageCostLabel ~= nil then
    fireMageCostLabel:setText("$" .. FireMageData.cost)
  end

  buyFireMageButton = UI:get("Fire Mage Button")
  if buyFireMageButton ~= nil then
    buyFireMageButton:onPointerClick(myScript, "buyFireMage")
  else
    print("Fire Mage Button not found")
  end

  thunderMageCostLabel = UI:get("Thunder Mage Cost Label")
  if thunderMageCostLabel ~= nil then
    thunderMageCostLabel:setText("$" .. ThunderMageData.cost)
  end

  buyThunderMageButton = UI:get("Thunder Mage Button")
  if buyThunderMageButton ~= nil then
    buyThunderMageButton:onPointerClick(myScript, "buyThunderMage")
  else
    print("Thunder Mage Button not found")
  end

  elderWizardCostLabel = UI:get("Elder Wizard Cost Label")
  if elderWizardCostLabel ~= nil then
    elderWizardCostLabel:setText("$" .. ElderWizardData.cost)
  end

  buyElderWizardButton = UI:get("Elder Wizard Button")
  if buyElderWizardButton ~= nil then
    buyElderWizardButton:onPointerClick(myScript, "buyElderWizard")
  else
    print("Elder Wizard Button not found")
  end

  eliteWarlockCostLabel = UI:get("Elite Warlock Cost Label")
  if eliteWarlockCostLabel ~= nil then
    eliteWarlockCostLabel:setText("$" .. EliteWarlockData.cost)
  end

  buyEliteWarlockButton = UI:get("Elite Warlock Button")
  if buyEliteWarlockButton ~= nil then
    buyEliteWarlockButton:onPointerClick(myScript, "buyEliteWarlock")
  else
    print("Elite Warlock Button not found")
  end

  particlesNode = scene:findNode("NumberParticles")
  if particlesNode ~= nil then
    particles = particlesNode:getComponent(ParticleSystemComponent)
    if particles ~= nil then
      particlesConfig = particles:getConfig()
    else
      print("NumberParticles component not found")
    end
  else
    print("NumberParticles not found")
  end
  
  camera = scene:getCamera()
  camera:setFollowNode(true)
  camera:setSize(Vector2f(1280, 720))
  camera:getOwner():transform():setPosition(Vector2f(640, 360))
  
  updateMoney()

  enemySpawnRate = startSpawnRate
end

function GameManager.onUpdate(self, deltaTime)
  mouseScreenPosition = Mouse:getPosition()
  mouseWorldPosition = camera:screenToWorld(Vector2i(mouseScreenPosition.x, mouseScreenPosition.y), Vector2i(1280, 720))

  --mouseInfo = string.format("Mouse World Position: %.2f, %.2f", mouseWorldPosition.x, mouseWorldPosition.y)
  --infoLabel:setText(mouseInfo)

  spawnCooldown = spawnCooldown - deltaTime
  if spawnCooldown <= 0 then
    spawnEnemy(self)
    
    enemySpawnRate = startSpawnRate / (math.max(1, playerDamage) + 1)

    spawnCooldown = enemySpawnRate
  end

end

function GameManager.onDestroyed(self)
end

function GameManager.toggleMenu(self)
  enabled = upgradesMenuContainer:isEnabled()
  upgradesMenuContainer:setEnabled(not enabled)
  upgradesMenuContainer:setVisible(not enabled)
end

function GameManager.buyCommonMage(self)
  onUnitBought(self, CommonMageData)
end

function GameManager.buyFireMage(self)
  onUnitBought(self, FireMageData)
end

function GameManager.buyThunderMage(self)
  onUnitBought(self, ThunderMageData)
end

function GameManager.buyElderWizard(self)
  onUnitBought(self, ElderWizardData)
end

function GameManager.buyEliteWarlock(self)
  onUnitBought(self, EliteWarlockData)
end

function GameManager.displayDamage(self, position, value)
  if particles ~= nil and particlesConfig ~= nil then
    particlesConfig.positionOffset = position
    particlesConfig.customData.id = value
    particles:emit(1, particlesConfig)
  end
end

function spawnEnemy(self)
  enemy = scene:createNode("Enemy")
  enemy:transform():setPosition(enemySpawnLocation)

  assetID = UUID.createFromName("CatOIIA.png")

  enemySprite = enemy:addComponent(SpriteComponent)
  enemySprite:setTextureAssetId(assetID)
  spriteSize = enemySprite:getPixelSize()
  spriteOrigin = Vector2f(spriteSize.x, spriteSize.y) * 0.5
  enemySprite:setOrigin(spriteOrigin)
  enemySprite:setScale(0.5)

  scriptID = UUID.createFromName("enemy.lua")
  enemyScriptComponent = enemy:addComponent(ScriptComponent, scriptID)
  enemyScript = enemyScriptComponent:instance()
  enemyScript.gameManager = self
  
  table.insert(Enemies, { transform = enemy:transform(), script = enemyScript })

  if self.targetEnemy == nil then
    self.targetEnemy = Enemies[1]
  end
end

function GameManager.enemyDestroyed(self, enemy)
  table.remove(Enemies, 1)
  self.targetEnemy = nil
  money = money + 1

  updateMoney()
  
  if #Enemies > 0 then
    self.targetEnemy = Enemies[1]
  else
    self.targetEnemy = nil
  end
end

function onUnitBought(self, unitData)
  amount = buySlider:getValue()
  totalCost = unitData.cost * amount

  if money >= totalCost then
    money = money - totalCost
    updateMoney()
  else
    return
  end
  
  for i = 1, amount do
    unit = scene:createNode("Unit")

    xOffset = math.random(0, 200)
    unit:transform():setPosition(unitSpawnLocation - Vector2f(-xOffset, unitData.yOffset))
    
    scriptID = UUID.createFromName("genericMage.lua")
    unitScriptComponent = unit:addComponent(ScriptComponent, scriptID)
    unitScript = unitScriptComponent:instance()
    
    unitScript.gameManager = self
    unitScript.spriteUUID = unitData.assetID
    unitScript.bulletDamage = unitData.damage

    playerDamage = playerDamage + unitData.damage
  end
end

function updateMoney()
  playerInfo = string.format("Money: %i", money)
  infoLabel:setText(playerInfo)
end

return GameManager