extends Vignette
## Disabling the controls of a busy screen grays them out. Making it non-interactable keeps its look.


func play() -> void:
	var stock_buy: Button = in_godot("Buy")
	var boxe_buy: Button = in_boxe("Buy")
	await move_both(stock_buy, boxe_buy, 0.7, Vector2(0.62, 0.6))
	godot_cursor.tap()
	boxe_cursor.tap()
	stock_buy.disabled = true
	in_boxe("Shop").interactable = false
	var tween := create_tween().set_parallel()
	for progress: ProgressBar in [in_godot("Progress"), in_boxe("Progress")]:
		tween.tween_property(progress, ^"value", 100.0, 1.8)
	await wait(0.9)
	# Clicking the busy screen is denied, without changing its look.
	godot_cursor.tap(Color("ff786b"))
	boxe_cursor.tap(Color("ff786b"))
	await tween.finished
	stock_buy.disabled = false
	in_boxe("Shop").interactable = true
	await wait(0.6)
