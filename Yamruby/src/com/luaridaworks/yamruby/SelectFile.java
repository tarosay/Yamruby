package com.luaridaworks.yamruby;

import java.io.File;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.Timer;
import java.util.TimerTask;

import android.app.AlertDialog;
import android.app.ListActivity;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.DialogInterface.OnDismissListener;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Matrix;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.os.Parcelable;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.view.Window;
import android.widget.ArrayAdapter;
import android.widget.EditText;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

public class SelectFile extends ListActivity {
	private static String IntentFile = "";			//選択されたファイル名パス
	private static File CurrentFolder = null;		//現在表示しているフォルダ

	private static final int REQUEST_GALLERY = 0;

	private ArrayList<String> prFiles = null;
	private ArrayList<Integer> prAttrs = null;		// 0:親フォルダ, 1:フォルダ, 2:ファイル
	private TextView prTitle = null;				//現在表示しているフォルダを表示するテキストビュ
	private int prSelBtn = 0;						//ダイアログボタン用
	private Timer prTimer = null;					//タイマを定義用
	private EditText prEditInput = null;			//入力ダイアログ用
	private Uri prPicture = null;					//取得画像のURI

	//*************************************************
	// 初期起動メソッド
	//*************************************************
	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);

		//アプリケーションタイトルを非表示
		requestWindowFeature(Window.FEATURE_NO_TITLE);

		//レイアウトの読み込み
		setContentView(R.layout.fileselect);

		//ファイル名取得配列と属性コントロールの初期化
		prFiles = new ArrayList<String>();
		prAttrs = new ArrayList<Integer>();

		if(CurrentFolder==null){
			//SDカードにあるsaridaフォルダのセット
			String SDCardDrive = Environment.getExternalStorageDirectory().getPath();
			String folder = SDCardDrive +  "/" + MainActivity.APPNAME;
			CurrentFolder = new File(folder);

			//AlertDialog dialog =
			new AlertDialog.Builder(this)
				.setTitle(getString(R.string.selFileSelectScript))
				.setMessage(getString(R.string.selFileShortcut))
				.setPositiveButton("OK", new DialogInterface.OnClickListener() {
					public void onClick(DialogInterface dialog, int whichButton) {
					}
				}).show();
		}

		//フォルダ内のディレクトリとスクリプトファイルを取得する
		if(getFiles(CurrentFolder)==true){

			//タイトルにフォルダ名のセット
			prTitle = (TextView)this.findViewById(R.id.title_list);
			prTitle.setText(CurrentFolder.getPath());

			//リストビュへセット
			ArrayAdapter<String> adapter = new ArrayAdapter<String>(this, android.R.layout.simple_list_item_1, prFiles);
			setListAdapter(adapter);
		}
	}

	//****************************************************
	// フォルダにあるファイルとディレクトリを取得します
	// prAttrs: 0:親フォルダ, 1:フォルダ, 2:ファイル
	//****************************************************
	private boolean getFiles(File folder){
		String ext = getString(R.string.selFileExt);
		String ext2 = getString(R.string.selFileExt2);
		
		File[] files = folder.listFiles();

		//nullのときは、リストはそのまま。
		if( files==null ){
			return false;
		}

		prFiles.clear();
		prAttrs.clear();

		if( folder.getPath().equals("/")==false){
			//ルートのときは、親は無い
			prFiles.add(".. <Parent>");
			prAttrs.add(0);
		}

		for(File file : files){
			if(file.isDirectory()){
				// ディレクトリのとき
				prFiles.add("<"+file.getName()+">");
				prAttrs.add(1);
			}
		}

		for(File file : files){
			if(!file.isDirectory()){
				//ファイルのとき
				String fname = file.getName().toLowerCase();
				
	    		if(	fname.endsWith("." + ext) || fname.endsWith("." + ext2) ){
					prFiles.add(file.getName());
					prAttrs.add(2);
	    		}
		    }
		}
		return true;
	}

	//*************************************************
	// リストが選択されたときに走るメソッド
	//*************************************************
	@Override
	protected void onListItemClick(ListView listView, View v, int position, long id) {
		super.onListItemClick(listView, v, position, id);

		if( prAttrs.get(position)==0){
			//親フォルダに戻る
			String folder = CurrentFolder.getParent();

			File selectFile = new File(folder);

			//ファイルの取得
			if(getFiles(selectFile)==true){
				//カレントフォルダの更新
				CurrentFolder = selectFile;
				//タイトルのセット
				prTitle.setText(CurrentFolder.getPath());

				//リストビュへセット
				ArrayAdapter<String> adapter = new ArrayAdapter<String>(this, android.R.layout.simple_list_item_1, prFiles);
				setListAdapter(adapter);
			}
		}
		else if(prAttrs.get(position)==1){
			//フォルダだった
			String folder = prFiles.get(position).substring(1);
			folder = folder.substring(0, folder.length() - 1);

			File selectFile = new File(CurrentFolder,folder);

			//ファイルの取得
			if(getFiles(selectFile)==true){
				//カレントフォルダの更新
				CurrentFolder = selectFile;
				//タイトルのセット
				prTitle.setText(CurrentFolder.getPath());

				//リストビュへセット
				ArrayAdapter<String> adapter = new ArrayAdapter<String>(this, android.R.layout.simple_list_item_1, prFiles);
				setListAdapter(adapter);
			}
		}
		else{
			//ファイルだったので、ショートカットを作ります
			IntentFile = prFiles.get(position);
			String[] items = { MainActivity.APPNAME + getString(R.string.selFileIcon),getString(R.string.selFileGetGazo)};
			prSelBtn = 0;
			AlertDialog dialog = new AlertDialog.Builder(this)
					.setIcon(R.drawable.ic_launcher)
					//.setTitle( prIntentFile + "のアイコンを選んでください" )
					.setTitle( getString(R.string.selFileSelectIcon) )
					.setItems(items, new DialogInterface.OnClickListener(){
						public void onClick(DialogInterface dialog, int which) {
							prSelBtn = which + 1;
						}
					}).show();
			dialog.setOnDismissListener(new OnDismissListener(){
				@Override
				public void onDismiss(DialogInterface dialog) {
					//ショートカット設定処理呼び出しタイマセット
					SetCallTimmer();
				}
			});
		}
	}

	//**************************************************
	// ショートカット設定処理呼び出しタイマ
	//**************************************************
	private void SetCallTimmer(){
		//ショートカット設定処理呼び出しタイマを設定する
		prTimer = new Timer(true);
		final android.os.Handler handler = new android.os.Handler();
		prTimer.schedule(
				new TimerTask() {
					@Override
					public void run() {
						handler.post( new Runnable(){
							public void run(){
								//タイマーを止めます
								prTimer.cancel();
								//ショートカット生成を呼び出す
								setShortcut();
							}
						});
					}
				}, 100, 100);  //初回起動の遅延と周期指定。単位はms
	}


	//**************************************************
	// ショートカット設定処理
	//**************************************************
	private void setShortcut() {
		//ファイル名入力の場合は別処理へ行く
		if(prSelBtn==2){
			//InputFilename();
			SelectImage();
			return;
		}

		//それ以外はショートカットの作成
		String ext = getString(R.string.selFileExt);
		String strText = "file://" + CurrentFolder.getPath() + "/" + IntentFile;

		// ショートカットに埋め込むインテントの生成
		Intent shortcutIntent = new Intent(Intent.ACTION_VIEW);
		shortcutIntent.setDataAndType( Uri.parse(strText), "x-" + MainActivity.APPNAME + "/" + ext );
		shortcutIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);

		// ショートカットをHOMEに作成する
		Intent intent = new Intent();
		intent.putExtra(Intent.EXTRA_SHORTCUT_INTENT, shortcutIntent);
		intent.putExtra(Intent.EXTRA_SHORTCUT_NAME, IntentFile);

		if( prSelBtn==1){
			Parcelable iconResource = Intent.ShortcutIconResource.fromContext(this, R.drawable.ic_launcher);
			intent.putExtra(Intent.EXTRA_SHORTCUT_ICON_RESOURCE, iconResource);
			intent.setAction("com.android.launcher.action.INSTALL_SHORTCUT");
			sendBroadcast(intent);

			Log.i(MainActivity.LOG_TAG,"Toast");
			Toast.makeText(getApplicationContext(), getString(R.string.selFileCreateScut), Toast.LENGTH_LONG).show();
		}
	}

	//**************************************************
	// 画像選択ギャラリーを呼び出します
	//**************************************************
	private void SelectImage() {
		Intent intent = new Intent();
		intent.setType("image/*");
		intent.setAction(Intent.ACTION_GET_CONTENT);
		startActivityForResult(intent, REQUEST_GALLERY);
	}

	//**************************************************
	// ギャラリー呼び出し結果が返ります
	//**************************************************
	@Override
	protected void onActivityResult(int requestCode, int resultCode, Intent data) {
		if(requestCode == REQUEST_GALLERY && resultCode == RESULT_OK) {

			if(IntentFile.equals("")==true){
				//アラートダイアログの呼び出しタイマを呼び出します
				AlertDialogCallTimmer("");
				return;
			}

			//画像のURIを保存します
			prPicture = data.getData();
			//アイコンサイズを入力します
			String strSize = "50";

			prEditInput = new EditText(this);
			prSelBtn = 0;
			prEditInput.setText(strSize);
			AlertDialog dialog = new AlertDialog.Builder(this)
			.setIcon(R.drawable.ic_launcher)
			.setTitle("アイコンサイズの入力")
			.setView(prEditInput)
			.setPositiveButton("OK", new DialogInterface.OnClickListener() {
				public void onClick(DialogInterface dialog, int whichButton) {
					// OKボタンをクリックした時の処理
					prSelBtn = 1;
				}
			})
			.setNegativeButton("Cancel", new DialogInterface.OnClickListener() {
				public void onClick(DialogInterface dialog, int whichButton) {
					// Cancel ボタンをクリックした時の処理
					prSelBtn = 0;
				}
			}).show();

			dialog.setOnDismissListener(new OnDismissListener(){
				@Override
				public void onDismiss(DialogInterface dialog) {

					if(prSelBtn==0){	return;	}

					int size =  50;
					try{
						size =  Integer.valueOf(prEditInput.getText().toString().trim());
					}
					catch(Exception e){
						size =  50;
					}

					Bitmap iconBitmap = null;

					iconBitmap = getBitmap(prPicture, size);

					if(iconBitmap==null){
						//アラートダイアログの呼び出しタイマを呼び出します
						AlertDialogCallTimmer(IntentFile);
						return;
					}

					// ショートカットに埋め込むインテントの生成
					String strText = "file://" + CurrentFolder.getPath() + "/" + IntentFile;
					Intent shortcutIntent = new Intent(Intent.ACTION_VIEW);
					shortcutIntent.setDataAndType( Uri.parse(strText), "x-" + MainActivity.APPNAME + "/" + getString(R.string.selFileExt) );
					shortcutIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);

					// ショートカットをHOMEに作成する
					Intent intent = new Intent();
					intent.putExtra(Intent.EXTRA_SHORTCUT_INTENT, shortcutIntent);
					intent.putExtra(Intent.EXTRA_SHORTCUT_NAME, IntentFile);

					intent.putExtra(Intent.EXTRA_SHORTCUT_ICON, iconBitmap);

					intent.setAction("com.android.launcher.action.INSTALL_SHORTCUT");
					sendBroadcast(intent);
					
					Log.i(MainActivity.LOG_TAG,"Toast");
					Toast.makeText(getApplicationContext(), getString(R.string.selFileCreateScut), Toast.LENGTH_LONG).show();
				}
			});
		}
	}

	//**************************************************
	// 画像ファイルを読み込みます
	//**************************************************
	private Bitmap getBitmap(Uri data,int iconsize) {

		Bitmap newbmp = null;
		try{
			InputStream inStream = getContentResolver().openInputStream(data);
			Bitmap bmp = BitmapFactory.decodeStream(inStream);
			inStream.close();
			if( bmp==null ){
				return bmp;
			}

			//縮小回数を読み込みます
			int n = 4;
			int w = bmp.getWidth();
			int h = bmp.getHeight();
			int neww = iconsize;
			int newh = iconsize;
			Matrix matrix = new Matrix();

			//n回の縮小処理
			float scw = (float)Math.exp(Math.log((float)neww/(float)w)/(float)n);
			float sch = (float)Math.exp(Math.log((float)newh/(float)h)/(float)n);
			matrix.postScale( scw, sch );
			System.gc();	//gcしてメモリを回収
			newbmp = Bitmap.createBitmap(bmp, 0, 0, w, h, matrix, true);
			bmp = null;
			for( int i=0; i<n-1; i++ ){
				newbmp = Bitmap.createBitmap(newbmp, 0, 0, newbmp.getWidth(), newbmp.getHeight(), matrix, true);
			}
		}
		catch(Exception e){

		}
		System.gc();	//gcしてメモリを回収
		return newbmp;
	}

	//**************************************************
	// 画像読み込みエラーアラートのタイマを設定する
	//**************************************************
	private void AlertDialogCallTimmer(final String filename) {
		//Timerを設定する
		prTimer = new Timer(true);
		final android.os.Handler handler = new android.os.Handler();
		prTimer.schedule(
				new TimerTask() {
					@Override
					public void run() {
						handler.post( new Runnable(){
							public void run(){
								//タイマーを止めます
								prTimer.cancel();
								//ショートカット生成を呼び出す
								dispAlert(filename);
							}
						});
					}
				}, 100, 100);  //初回起動の遅延と周期指定。単位はms
	}

	//**************************************************
	// 画像読み込みエラーアラートを出す
	//**************************************************
	private void dispAlert(String filename) {
		String mess = "";

		if(filename.equals("")==true){
			mess = getString(R.string.selFileErrRead) + "\n" + getString(R.string.selFileRetry);
		}
		else{
			mess = filename + getString(R.string.selFileNotRead);
		}

		//アラートダイアログを出す
		new AlertDialog.Builder(this)
		.setTitle(getString(R.string.selFileErrTitle))
		.setMessage(mess)
		.setPositiveButton("OK", new DialogInterface.OnClickListener() {
			public void onClick(DialogInterface dialog, int whichButton) {
			}
		}).show();
	}

	//**************************************************
	// バックキーが押されたら終了する
	//**************************************************
	@Override
	public boolean onKeyDown(int keyCode, KeyEvent event) {
		if(keyCode==KeyEvent.KEYCODE_BACK){

			finish();

			return true;
		}
		return false;
	}
}