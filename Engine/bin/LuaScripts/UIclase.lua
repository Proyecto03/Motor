local UIclase = {}

UIclase["instantiate"] = function (params, entity)
    --p = JSON:decode(params)
    local self = {}
    self.entity = entity
    print("Hola tiwi instantiate")
    return self
end

UIclase["start"] = function (_self, lua)
    _self.button = lua:getUIButton(_self.entity)
    print("Hola tiwi start")
end

UIclase["update"] = function (_self, lua)
    if _self.button:getButtonPressed() == true then
        print("Hola tiwi")
        _self.button:buttonNotPressed()
    end


end

return UIclase