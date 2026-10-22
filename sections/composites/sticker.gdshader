shader_type canvas_item;
render_mode unshaded;

uniform sampler2D screen_texture : hint_screen_texture, repeat_disable, filter_linear;
uniform vec4 outline_color : source_color = vec4(1.0);
uniform float outline_width : hint_range(0.0, 12.0) = 5.0;
uniform vec4 shadow_color : source_color = vec4(0.0, 0.0, 0.0, 0.45);
uniform vec2 shadow_offset = vec2(3.0, 6.0);

float coverage(vec2 uv, vec2 pixel) {
	float alpha = 0.0;
	for (int i = 0; i < 16; i++) {
		float angle = float(i) * TAU / 16.0;
		alpha = max(alpha, textureLod(screen_texture, uv + vec2(cos(angle), sin(angle)) * outline_width * pixel, 0.0).a);
	}
	return alpha;
}

void fragment() {
	vec4 color = textureLod(screen_texture, SCREEN_UV, 0.0);
	if (color.a > 0.0001) {
		color.rgb /= color.a;
	}
	float outline = max(color.a, coverage(SCREEN_UV, SCREEN_PIXEL_SIZE));
	float shadow = coverage(SCREEN_UV - shadow_offset * SCREEN_PIXEL_SIZE, SCREEN_PIXEL_SIZE);

	vec4 result = vec4(shadow_color.rgb, shadow_color.a * shadow);
	result = mix(result, vec4(outline_color.rgb, 1.0), outline * outline_color.a);
	result = mix(result, vec4(color.rgb, 1.0), color.a);
	result.a = max(max(shadow * shadow_color.a, outline * outline_color.a), color.a);
	COLOR *= result;
}
