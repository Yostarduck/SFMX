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

-- Owned units
local Enemies       = {}
local CommonMages   = {}
local FireMages     = {}
local ThunderMages  = {}
local ElderWizards  = {}
local EliteWarlocks = {}

-- Units
local CommonMageData   = { cost = 1,    assetID = UUID.createFromName("CatDrooling.png"),  yOffset = 0   }
local FireMageData     = { cost = 3,    assetID = UUID.createFromName("CatHappy.png"),     yOffset = 100 }
local ThunderMageData  = { cost = 50,   assetID = UUID.createFromName("CatPop.png"),       yOffset = 200 }
local ElderWizardData  = { cost = 1000, assetID = UUID.createFromName("CatTongue.png"),    yOffset = 300 }
local EliteWarlockData = { cost = 5000, assetID = UUID.createFromName("CatSoldier.png"),   yOffset = 400 }

-- Game Settings
local enemySpawnLocation = Vector2f(1280, 580)
local unitSpawnLocation = Vector2f(50, 580)
local money = 1

function GameManager.onCreated(self)
  self.targetEnemy = nil
end

function GameManager.onStart(self)
  scene = SceneManager:getActiveScene()
  
  myScript = self.owner:getComponent(ScriptComponent)
  
  infoLabelNode = scene:findNode("InfoLabel")
  if infoLabelNode ~= nil then
    infoLabel = infoLabelNode:getComponent(UILabel)
  else
    print("infoLabel not found")
  end
  
  upgradesMenuButtonNode = scene:findNode("UpgradesButton")
  if upgradesMenuButtonNode ~= nil then
    upgradesMenuButton = upgradesMenuButtonNode:getComponent(UIButton)

    upgradesMenuButton:onPointerClick(myScript, "toggleMenu")
  else
    print("UpgradesButton not found")
  end
  
  upgradesMenuContainerNode = scene:findNode("UpgradesMenu")
  if upgradesMenuContainerNode ~= nil then
    upgradesMenuContainer = upgradesMenuContainerNode:getComponent(UIScrollView)
    
    upgradesMenuContainer:setEnabled(false)
    upgradesMenuContainer:setVisible(false)
  else
    print("UpgradesMenu not found")
  end

  buySliderNode = scene:findNode("BuySlider")
  if buySliderNode ~= nil then
    buySlider = buySliderNode:getComponent(UISlider)
  else
    print("BuySlider not found")
  end
  
  buyCommonMageNode = scene:findNode("Common Mage Button")
  if buyCommonMageNode ~= nil then
    buyCommonMageButton = buyCommonMageNode:getComponent(UIButton)

    buyCommonMageButton:onPointerClick(myScript, "buyCommonMage")
  else
    print("Common Mage Button not found")
  end
  
  buyFireMageNode = scene:findNode("Fire Mage Button")
  if buyFireMageNode ~= nil then
    buyFireMageButton = buyFireMageNode:getComponent(UIButton)

    buyFireMageButton:onPointerClick(myScript, "buyFireMage")
  else
    print("Fire Mage Button not found")
  end
  
  buyThunderMageNode = scene:findNode("Thunder Mage Button")
  if buyThunderMageNode ~= nil then
    buyThunderMageButton = buyThunderMageNode:getComponent(UIButton)

    buyThunderMageButton:onPointerClick(myScript, "buyThunderMage")
  else
    print("Thunder Mage Button not found")
  end
  
  buyElderWizardNode = scene:findNode("Elder Wizard Button")
  if buyElderWizardNode ~= nil then
    buyElderWizardButton = buyElderWizardNode:getComponent(UIButton)

    buyElderWizardButton:onPointerClick(myScript, "buyElderWizard")
  else
    print("Elder Wizard Button not found")
  end
  
  buyEliteWarlockNode = scene:findNode("Elite Warlock Button")
  if buyEliteWarlockNode ~= nil then
    buyEliteWarlockButton = buyEliteWarlockNode:getComponent(UIButton)

    buyEliteWarlockButton:onPointerClick(myScript, "buyEliteWarlock")
  else
    print("Elite Warlock Button not found")
  end
  
  camera = scene:getCamera()
  camera:setFollowNode(true)
  camera:setSize(Vector2f(1280, 720))
  camera:getOwner():transform():setPosition(Vector2f(640, 360))
  
  updateMoney()

  spawnEnemy(self)
end

function GameManager.onUpdate(self, deltaTime)
  mouseScreenPosition = Mouse:getPosition()
  mouseWorldPosition = camera:screenToWorld(Vector2i(mouseScreenPosition.x, mouseScreenPosition.y), Vector2i(1280, 720))

  --mouseInfo = string.format("Mouse World Position: %.2f, %.2f", mouseWorldPosition.x, mouseWorldPosition.y)
  --infoLabel:setText(mouseInfo)
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

  self.targetEnemy = { transform = enemy:transform(), script = enemyScript }
end

function GameManager.enemyDestroyed(self, enemy)
  self.targetEnemy = nil
  money = money + 1

  updateMoney()

  spawnEnemy(self)
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
  end
end

function updateMoney()
  playerInfo = string.format("Money: %i", money)
  infoLabel:setText(playerInfo)
end

return GameManager