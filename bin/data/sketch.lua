-- TrussSketch (Lua) PoC sample — intentionally simple, uses only core bindings.

local t = 0

function setup()
    t = 0
end

function update()
    t = t + getDeltaTime()
end

function draw()
    clear(0.1)

    local w = getWindowWidth()
    local h = getWindowHeight()

    -- orbiting circle
    setColor(0.3 + 0.3 * math.sin(t), 0.6, 0.95)
    local x = w / 2 + math.cos(t) * 150
    local y = h / 2 + math.sin(t * 1.3) * 120
    drawCircle(x, y, 40)

    -- a row of bars
    for i = 0, 9 do
        local bh = 30 + math.abs(math.sin(t * 2 + i * 0.4)) * 120
        setColor(0.9, 0.4 + i * 0.05, 0.3)
        drawRect(40 + i * 52, h - bh - 30, 40, bh)
    end

    -- mouse dot
    setColor(1, 1, 1)
    drawCircle(getMouseX(), getMouseY(), 10)
end

function keyPressed(key)
    print("key pressed: " .. key)
end
