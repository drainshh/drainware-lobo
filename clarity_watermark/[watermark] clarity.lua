// clarity.tk V2 Watermark
// by checha

local FrameTime = FrameTime
local LocalPlayer = LocalPlayer
local CurTime = CurTime
local math_Round = math.Round
local math_abs = math.abs
local math_max = math.max
local ScrW = ScrW
local ScrH = ScrH
local set_scissor_rect = render.SetScissorRect
local box = draw.RoundedBox
local nfps = math_Round(1 / FrameTime(), 0)

local SetMaterial = surface.SetMaterial
local SetTextPos = surface.SetTextPos
local SetDrawColor = surface.SetDrawColor
local SetTextColor = surface.SetTextColor
local DrawText = surface.DrawText
local DrawTexturedRect = surface.DrawTexturedRect
local SetFont = surface.SetFont
local GetTextSize = surface.GetTextSize

local accent_color = Color(108, 127, 213)

local clarity_in = Color(16, 15, 15)
local clarity_in1 = Color(81, 79, 79)
local clarity_out = Color(27, 25, 25)
local gray_color = Color(20, 20, 20)

local i_color = Color(60, 60, 60)
local text_color = Color(138, 138, 138)
local fps_color = Color(229, 229, 234)

surface.CreateFont( "clarity_font", {
	font = "Montserrat SemiBold",
	size = 18,
	weight = 400,
	antialias = true,
    additive = false,
})

local text_x = 0
local text_additive = {}

local function clarity_balpha(color, alpha)
    return Color(color.r, color.g, color.b, alpha)
end

local function clarity_text(text, color, alpha, x, y, id, noresize)
    id = id or text

    local w, h = GetTextSize(text)

    SetTextColor(clarity_balpha(color, alpha))
    SetTextPos(x, y)

    DrawText(text)

    if not text_additive[id] and not noresize then
        text_x = text_x + w
        text_additive[id] = true
    end
end

local function clarity_rounded(radius, x, y, w, h, color, rounded)
    rounded = rounded or 8

    box(rounded, x - radius, y - radius, w + (radius * 2), h + (radius * 2), color)
end

local function clarity_image(img, color, alpha, x, y)
    SetDrawColor(clarity_balpha(color, alpha))
    SetMaterial(img)
    DrawTexturedRect(x, y, 65, 65)
end

local image = Material("clarity.png", "noclamp smooth")
local width, alpha = 0, 0
local fps_delay = 0

local function clarity_fps()
    if fps_delay - CurTime() > 0 then
        return
    end

    nfps = math_Round(1 / FrameTime(), 0)
    fps_delay = CurTime() + 0.5
end

local function clarity_watermark()
    local lply = LocalPlayer()
    local nick = lply:Nick()
    local fps = nfps

    local w, h = text_x - 20, 30

    SetFont("clarity_font")

    local nick_w = GetTextSize(nick)
    local fps_w = GetTextSize(fps)

    local total_w = w + nick_w + fps_w
    local x, y = (ScrW() - width) - 10, 10

    width = Lerp(1.4 * FrameTime(), width or 0, total_w)

    if width > (total_w - 50) then
        alpha = Lerp(1.5 * FrameTime(), alpha or 0, 255)
    end

    clarity_rounded(4, x, y, width, h, clarity_out)
    clarity_rounded(3, x, y, width, h, clarity_in1)
    clarity_rounded(2, x, y, width, h, clarity_in)
    box(6, x, y, width, h, gray_color)

    set_scissor_rect(x, y, x + w, y + h, true)
        clarity_image(image, accent_color, math_max(alpha - 200, 0), x - 20, y - 20)
    set_scissor_rect(0, 0, 0, 0, false)

    clarity_text("clarity", accent_color, alpha, x + 12 - 5, y + 5)
    clarity_text("|", i_color, alpha, x + 60 - 5, y + 4.5)
    clarity_text(nick, text_color, alpha, x + 68 - 5, y + 5)

    clarity_text("|", i_color, alpha, x + 72 + nick_w - 5, y + 4.5)
    clarity_text(fps, fps_color, alpha, x + 80 + nick_w - 5, y + 5, "count_fps")

    clarity_text("fps", text_color, alpha, x + 85 + nick_w + fps_w - 5, y + 5)
end

local cp_accent = cp_accent or nil

hook.Add("Tick", "Clarity-FPS", clarity_fps)
hook.Add("DrawOverlay", "Clarity-Watermark", clarity_watermark)

concommand.Add("accent", function()
    if IsValid(cp_accent) then
        cp_accent:Remove()
        cp_accent = nil
        return
    end

    cp_accent = vgui.Create("DColorMixer")
    cp_accent:SetPos((ScrW() - 200) / 2, (ScrH() - 200) / 2)
    cp_accent:SetPalette(false)
    cp_accent:SetAlphaBar(false)
    cp_accent:SetWangs(false)
    cp_accent:SetColor(accent_color)
    cp_accent:MakePopup()
    cp_accent.ValueChanged = function(self, col)
        accent_color = col
    end
end)