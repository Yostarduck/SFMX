-- Simple Lua script to generate entities
-- File: entity.lua

local m_scene
local m_spriteComponent
local m_entityType = 0
local m_health = 0
local m_damage = 0
local m_speed = 0.0

local Entity = {}

function setData(type, health, damage, speed, spriteID)
  m_entityType = type
  m_health = health
  m_damage = damage
  m_speed = speed
  m_spriteComponent = self:addComponent(SpriteComponent)
end

function Entity.onCreated(self)

end

function Entity.onStart(self)
end

function Entity.onUpdate(self, deltaTime)
end

return Entity