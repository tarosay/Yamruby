#-----------------------------------------
# Readme
#-----------------------------------------
$Text = Array[
	"　Ya mrubyは、mrubyをAndroidに実装したものです。Ya mruby単体ではmrubyが動作するのみで、Androidを操作することはできません。",
	"　Androidを操作するためには、Marida2という別アプリが必要となります。Marida2はSocketを経由してAndroidの操作ができるツールです。",
	"　Ya mubyは、このMarida2を利用してAndroidを操作すると仕組みとなっています。",
	"　Ya mruby実行すると、yamruby.rb等いくつかのスクリプトがSDメモリカードの/Yamruby下にコピーされ、Yamruby.rbが実行されます。",
	"　プログラムを自作したい場合は、yamruby.rbにプログラムを書くか、Yamrubyのショートカットを画面に作り、自作rbファイルを指定してください。",
	"　コマンドの解説など詳しくは、公開しているYa mrubyのソースを見てください。",
	"　基本的に、私が公開しているLuaridaに実装しているコマンド郡と同じものが使えます。",
	"　また、Androidのエディタを使ってスクリプトを書くこともできます。Jota Text Editorがおすすめです。",
	"　これを使えば、rubyスクリプトの直実行も可能です。"
	]
#-----------------------------------------
# メインプログラム
#-----------------------------------------
def main()
	Canvas.drawCls( color(255,255,255) )
	w,h = Canvas.getviewSize()

	y = 0
	$Text.each{|txt|
		y = y + Canvas.drawTextBox( txt, 0, y*26, 24, color(0,0,0) , w )
	}

	y = y + Canvas.drawTextBox( "　ソース一式は githubにあります。", 0, y*32, 30, color(0,0,0) , w )
	Canvas.drawTextBox( "https://github.com/tarosay/Yamruby", 0, y*32, 30, color(0,0,255) , w )
	touch(3)
end
main()
#System.exit()