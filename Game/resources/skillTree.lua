-- File: skillTree.lua
-- Lua script that defines the skill tree for the game

local SkillTree = {}

function SkillTree.onCreated(self)
  skillNodeScriptID = UUID.createFromName("skillNode.lua")
  self.treeRoot = self.owner:addComponent(ScriptComponent, skillNodeScriptID)
  
end

function SkillTree.onStart(self)
end

function SkillTree.onUpdate(self, deltaTime)
end

function SkillTree.onDestroyed(self)
end



return SkillTree