Bootstrap = {}

Bootstrap.scripts = {}

Bootstrap.scripts[0] = "test.lua"
Bootstrap.scripts[1] = "some.lua"

Bootstrap.num_of_scripts = function()
  local count = 0
  for _ in pairs( Bootstrap.scripts ) do count = count + 1 end
  return count
end