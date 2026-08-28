-- file: inputDemo.lua
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
