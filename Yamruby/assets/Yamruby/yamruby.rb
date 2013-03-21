#------------------------------------------
# アプリ選択メニュ v0.01
#------------------------------------------
#-グローバル変数宣言----------------------
$Ver = "v0.01"
$Menu = [
	["Read Me", "readme.rb"],
	["終了する(EXIT)", "exit.rb"]
	]
$MrubyPath = System.getCardMnt() + "/Yamruby"
#-----------------------------------------
# メインプログラム
#-----------------------------------------
def main()
	Canvas.drawCls( color(0,0,255) )
	Item.clear()
	$Menu.each{|mnu|
		Item.add( mnu[0], 0 )
	}

	# リスト選択
	a = Item.list( "Ya mruby Ver." + System.version() )
	if( a>0 )then
		System.setrun( $MrubyPath+"/"+$Menu[a-1][1] )
	else
		dialog( "Ya mrubyを終了します", "また使ってね", 1 )
		#Yamrubyを強制終了します
		System.exit()
	end
end
main()
#System.exit()