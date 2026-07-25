
local m_prefab = nil
local m_spawnInterval = 0.0
local m_spawnTimer = 0.0
local m_spawnCount = 0

local EntitySpawner = {}

function spawnEntity(self, id)
 local entityNode = scene:createNode("Entity")
 local currentPosition = self:transform():getPosition()
 entityNode:transform():setPosition(currentPosition)
 local entityUUID = UUID.createFromName("entity.lua")
 local entityScript = entityNode:addComponent(ScriptComponent, entityUUID)
end

return EntitySpawner