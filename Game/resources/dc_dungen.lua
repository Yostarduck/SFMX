local DCDunGen = {}

function DCDunGen.onCreated(self)
  self.dungeonWidth   = 10
  self.dungeonHeight  = 10
  self.roomMinSize    = 3
  self.roomMaxSize    = 6

  

  self.floors = {}
end

function DCDunGen.onStart(self)
end

function DCDunGen.onUpdate(self, deltaTime)
end

function DCDunGen.onDestroyed(self)
end

function DCDunGen.generateDungeon(self)
end

return DCDunGen