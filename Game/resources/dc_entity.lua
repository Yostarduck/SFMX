local DCEntity = {}

function DCEntity.onCreated(self)
  self.life = 100
  self.destructible = true
  self.gameManager = nil
  self.speed = 0
  self.damage = 0
  self.activeItem = nil
  self.target = nil
  self.sprite = nil
  self.animation = nil
  self.collider = nil

end

function DCEntity.onStart(self)
end

function DCEntity.onUpdate(self, deltaTime)
end

function DCEntity.onDestroyed(self)
end

function DCEntity.damage(self, amount)

end

return DCEntity