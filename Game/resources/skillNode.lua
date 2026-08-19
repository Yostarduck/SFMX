-- File: skillNode.lua
-- Lua script that defines a skill node for the skill tree ingame

local SkillNode = {}

SkillNode.childrenNodes = {} 

function SkillNode.onCreated(self)
  self.sprite = self.owner:addComponent(SpriteComponent)
  self.sprite:setTextureAssetId(UUID.createFromName("skillNode.png"))
end

function SkillNode.onStart(self)
end

function SkillNode.onUpdate(self, deltaTime)
end

function SkillNode.onDestroyed(self)
end



return SkillNode