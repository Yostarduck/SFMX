-- file: inputDemo.lua
-- Read/subscribe example for the mapping-mode input system.
--
-- The runtime authoring surface (for creating and rebinding mappings) looks like:
--
--   local mapping = InputSystem:createMapping("MyControls")
--   local player  = mapping:addMap("Player")
--   local jump    = player:addAction("Jump", ActionValueType.Button)
--   assert(jump:bind("Keyboard/Space"))            -- validated control path
--   assert(jump:bind("Gamepad0/Button/South"))
--   jump:bindComposite({                            -- e.g. WASD + left stick
--     { path = "Keyboard/A",          role = CompositeRole.NegativeX },
--     { path = "Keyboard/D",          role = CompositeRole.PositiveX },
--     { path = "Gamepad0/Axis/LeftX", role = CompositeRole.AxisX },
--   })
--   jump:setInteraction(InteractionType.Press)
--   jump:addProcessor(ProcessorType.DeadZone, 0.15, 0.95)
--   InputSystem:setActiveMapping(mapping)
--   -- ...
--   InputSystem:destroyMapping(mapping)              -- clears it if active
--
-- Control paths are "Keyboard/<Key>", "Mouse/<Button>",
-- "GamepadN/Axis/<Axis>" and "GamepadN/Button/<Button>".
-- InputControl.fromPath(path) returns nil for a malformed path, while
-- jump:bind(path) returns false and adds nothing.
--
-- Shows the mapping-mode input bindings: reach an action, subscribe a callback,
-- and unbind a specific subscription. Submit stays bound; Cancel unbinds itself
-- the first time it fires.
local InputDemo = {}

function InputDemo.onStart(self)
  self.me = self.owner:getComponent(ScriptComponent)

  local mapping = InputSystem:getActiveMapping()
  if mapping == nil then
    print("[inputDemo] no active mapping")
    return
  end

  local ui = mapping:findMap("UI")
  if ui == nil then
    print("[inputDemo] no UI map")
    return
  end

  local submit = ui:findAction("Submit")
  if submit ~= nil then
    self.submitToken = submit:onPerformed(self.me, "onSubmit")
  end

  local cancel = ui:findAction("Cancel")
  if cancel ~= nil then
    self.cancelToken = cancel:onPerformed(self.me, "onCancel")
  end
end

function InputDemo.onSubmit(self, ctx)
  print(string.format("[inputDemo] Submit phase=%d value=%s",
    ctx.phase, tostring(ctx.value:asBool())))
end

function InputDemo.onCancel(self, ctx)
  print("[inputDemo] Cancel fired once; unbinding it now")
  if self.cancelToken ~= nil then
    self.me:unbind(self.cancelToken)
    self.cancelToken = nil
  end
end

function InputDemo.onDestroyed(self)
  self.me:unbindAll()
end

return InputDemo
