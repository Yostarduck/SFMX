local Enemy = DCEntity:new()

function Enemy.onCreated(self)
  self.lookoutRange = 0
  self.attackRange  = 0
end

function Enemy.onStart(self)
end

function Enemy.onUpdate(self, deltaTime)
  local player
  self:lookout(self, player)
end

function Enemy.onDestroyed(self)
  -- Cleanup
end

function Enemy.damage(self, amount)
  -- Attack player, should call damage function of player
end

function Enemy.damaged(self, amount)
  -- Set enemy life and check if he should die
end

function Enemy.lookout(self, target)
  -- Check out for player and see if its within range
end

