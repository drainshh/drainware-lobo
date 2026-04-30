

	//// === ICON (PARTIALLY CLIPPED, SAME AS CLARITY) ===
	//{
	//	ImGui::PushFont(fonts::logo_watermark); // same icon font

	//	const char* icon = "A";
	//	ImVec2 icon_size = ImGui::CalcTextSize(icon);

	//	// same visible ratio as clarity
	//	const float visible_ratio = 0.7f;

	//	// position identical logic
	//	ImVec2 icon_pos(
	//		top_left.x + 6.f - (icon_size.x * (1.f - visible_ratio)),
	//		text_pos.y - 25.f
	//	);

	//	// clip strictly to watermark bounds
	//	draw_list->PushClipRect(top_left, bottom_right, true);
	//	draw_list->AddText(icon_pos,
	//		ImColor(
	//			menu::menu_accent[0],
	//			menu::menu_accent[1],
	//			menu::menu_accent[2],
	//			0.15f // same subtle fade as clarity
	//		),
	//		icon
	//	);
	//	draw_list->PopClipRect();

	//	ImGui::PopFont();
	//}

void features::visuals::clarity_watermark() {
	if (!c::movement::billware_wm)
		return;

	if (!(c::movement::watermark_type == 1))
		return;

	static float frame_rate = 0.f;
	static float last_update_time = 0.f;
	static const float update_interval = 0.5f;

	float current_time = interfaces::globals->realtime;

	// prevent division by zero
	float ft = interfaces::globals->frame_time;
	if (ft <= 0.f)
		ft = 0.0001f;

	if (current_time - last_update_time >= update_interval) {
		const float alpha = 0.7f;
		frame_rate = alpha * frame_rate + (1.f - alpha) * (1.f / ft);

		last_update_time = current_time;
	}

	int fps = static_cast<int>(frame_rate);
	std::string fps_number = std::to_string(fps);
	std::string fps_label = " fps";

	std::string part1 = "clarity";
	std::string sep1 = " | ";
	std::string part2 = "dev";
	std::string sep2 = " | ";

	int w, h;
	interfaces::engine->get_screen_size(w, h);

	std::string full_text = part1 + sep1 + part2 + sep2 + fps_number + fps_label;
	auto text_size = im_render.measure_text(full_text.c_str(), fonts::clarity_watermark, 15.f);
	static const ImVec2 padding = ImVec2(7, 7);
	static const ImVec2 margin = ImVec2(3, 3);

	ImVec2 text_pos = ImVec2(w - text_size.x - padding.x - margin.x, padding.y + margin.y);

	ImVec2 top_left(w - text_size.x - padding.x - margin.x * 2.f - 2.f, padding.y - 2.f);
	ImVec2 bottom_right(w - padding.x + 2.f, text_size.y + padding.y + margin.y * 2.f + 2.f);

	// === BACKGROUND DRAWING ===
	ImColor accent(menu::menu_accent[0], menu::menu_accent[1], menu::menu_accent[2], 1.f);
	ImColor accent2(menu::menu_accent[0], menu::menu_accent[1], menu::menu_accent[2], 0.15f);
	ImColor darkgrey(0.5f, 0.5f, 0.5f, 1.f);
	ImColor darkgrey2(0.2f, 0.2f, 0.2f, 1.f);
	ImColor white(0.8f, 0.8f, 0.8f, 1.f);
	ImColor fg_color(10, 10, 12, 255);
	ImColor fg2_color(10, 10, 12, 255);
	ImColor bg_color(0.14f, 0.14f, 0.14f, 1.f);
	ImColor bgfade_color(10, 10, 12, 50);
	ImColor bg2_color(20, 20, 22, 250);
	ImColor bg3_color(4, 4, 5, 100);

	ImDrawList* draw_list = ImGui::GetBackgroundDrawList();

	draw_list->AddRectFilledMultiColorRounded(
		top_left, bottom_right,
		bgfade_color, fg2_color, fg2_color, bg_color, bg_color,
		5.5f, ImDrawFlags_RoundCornersAll
	);
	draw_list->AddRectFilledMultiColorRounded(
		top_left, bottom_right,
		bgfade_color, bg_color, bg_color, fg2_color, fg2_color,
		5.5f, ImDrawFlags_RoundCornersAll
	);
	draw_list->AddRect(top_left, bottom_right, bg2_color, 5.5f);
	draw_list->AddRect(
		{ top_left.x - 1.f, top_left.y - 1.f },
		{ bottom_right.x + 1.f, bottom_right.y + 1.f },
		bg3_color, 5.5f
	);
	draw_list->AddRect(
		{ top_left.x - 2.5f, top_left.y - 2.5f },
		{ bottom_right.x + 2.5f, bottom_right.y + 2.5f },
		bg2_color, 5.5f
	);

	// === TEXT DRAWING ===
	ImGui::PushFont(fonts::clarity_watermark);

	ImVec2 cursor = text_pos;

	// === ICON (PARTIALLY CLIPPED, NO BOX) ===
	ImGui::PushFont(fonts::watermark_icons);

	const char* icon = "A";
	ImVec2 icon_size = ImGui::CalcTextSize(icon);

	// show only ~70% of icon width
	const float visible_ratio = 0.7f;
	const float advance_x = icon_size.x * visible_ratio;

	// draw icon shifted left so part is clipped
	ImVec2 icon_pos(
		top_left.x + 6.f - (icon_size.x * (1.f - visible_ratio)),
		cursor.y - 17.f
	);

	// clip strictly to watermark bounds
	draw_list->PushClipRect(top_left, bottom_right, true);
	draw_list->AddText(icon_pos, accent2, icon);
	draw_list->PopClipRect();