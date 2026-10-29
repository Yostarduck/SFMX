
local DCPlayer = DCEntity:new()

function DCPlayer.onCreated(self)
end

function DCPlayer.onStart(self)
end

function DCPlayer.onUpdate(self, deltaTime)
end

function DCPlayer.onDestroyed(self)
end

function DCPlayer.damage(self, amount)
  -- Attack player, should call damage function of player
end

function DCPlayer.damaged(self, amount)
end

function DCPlayer.move(self, direction, deltaTime)
end

function DCPlayer.addItem(self, item)
end

function DCPlayer.removeItem(self, item)
end

function DCPlayer.useItem(self, item)
end

function DCPlayer.action(self, target)
end



return DCPlayer


