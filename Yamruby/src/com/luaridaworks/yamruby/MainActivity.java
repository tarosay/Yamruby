package com.luaridaworks.yamruby;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.os.Process;
import android.app.Activity;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.res.AssetManager;
import android.util.Log;
import android.view.Menu;
import android.view.Window;
import android.widget.Toast;

public class MainActivity extends Activity {

	public static final String LOG_TAG = "com.luaridaworks.yamruby";
	public static String APPNAME = "Yamruby";

	private String SDCardDrive = "";
	private String ScriptFilename = "";
    private boolean RunningShortcut = false;

    public native int VMStart(String sdcarddrive, String filename);
    static{ System.loadLibrary("vm_module"); }

	
	//============================================
	// Create
	//============================================
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		//setContentView(R.layout.activity_main);

		//アプリケーションタイトルを非表示
		requestWindowFeature(Window.FEATURE_NO_TITLE);

		//SDカードのセットアップ
		//SDカードがあるかどうかのチェック無かったら終わり
		if( sdcardInit()==false){
			finish();
			//return;
		}		
	}

	//============================================
	// SDカードにデータをセットアップします。
	//============================================
	private boolean sdcardInit() {
		String folder = APPNAME;
		
		String status = Environment.getExternalStorageState();
		if (status.equals(Environment.MEDIA_MOUNTED)) {
			//SDカードがマウントされているときは、その名称をpsSDCardDriveに入れる
			SDCardDrive = Environment.getExternalStorageDirectory().getPath();
		}
		else{
			Toast.makeText(getApplicationContext(), getString(R.string.errNotMountSD), Toast.LENGTH_LONG).show();
			return(false);
		}

		//フォルダがあるか、確認します。あれば、終了します。無いときには、フォルダを作りサンプルを転送します。
		File SDfolder = new File(SDCardDrive + "/" + folder);

		//フォルダの存在を確認します
		if( SDfolder.exists()){
			//存在したら、startFilenameの存在を確認します。
			File file = new File(SDCardDrive + "/" + folder + "/" + getString(R.string.startFilename));
			if( file.exists()){	return( true );	}	//存在したら、何もしないで戻ります
		}
		else{
			//フォルダが無いときにはフォルダを作成します。
			if (!SDfolder.mkdir()){
				Toast.makeText(getApplicationContext(), getString(R.string.errMkDirSD), Toast.LENGTH_LONG).show();
				return(false);
			}
		}

		//assetのファイルを、sdcardにコピーします
		try {
			AssetManager as = getResources().getAssets();
			String[] saridafiles = as.list(folder);	//assets以下にあるファイル名を読み込みます
			int n;
			int DEFAULT_BUFFER_SIZE = 1024 * 4;
			byte[] buffer = new byte[DEFAULT_BUFFER_SIZE];

			for(int i=0; i<saridafiles.length; i++ ){
				InputStream is = as.open(folder+"/"+saridafiles[i]);
				String toFile = SDCardDrive + "/" + folder + "/" + saridafiles[i];

				File outfile = new File(toFile);
				OutputStream output = new FileOutputStream(outfile);

				n = 0;
				while (-1 != (n = is.read(buffer))) {
					output.write(buffer,0,n);
				}
				output.flush();
				output.close();
				is.close();
			}
			
		} catch (IOException e) {
			e.printStackTrace();
		}

		return(true);
	}

	//============================================
	// Resume
	//============================================
	@Override
	public void onResume(){
		super.onResume();

		Log.d(LOG_TAG, "Intent= " + getIntent().getAction());
		int shortcutFount = 0;
		
		if(Intent.ACTION_MAIN.equals(getIntent().getAction())){
			shortcutFount += 1;
		}

		if(Intent.ACTION_VIEW.equals(getIntent().getAction())
				|| Intent.ACTION_MAIN.equals(getIntent().getAction())) {

			Uri data = getIntent().getData();
			if (data != null){
				ScriptFilename = data.getPath();
			}
			else{
				shortcutFount += 2;
				//Toast.makeText(getApplicationContext(), getString(R.string.errShortcut1), Toast.LENGTH_LONG).show();
				ScriptFilename = SDCardDrive + "/" + APPNAME + "/" + getString(R.string.startFilename);
			}

			//保存していたPIDを取得する
			SharedPreferences sp = getSharedPreferences("yamruby", MODE_PRIVATE);
			int pid = sp.getInt("PID", -1);
			if(pid!=-1){
				if(pid!=android.os.Process.myPid()){
					//前回実行したmrubyVMプロセスを止めます
					Process.killProcess(pid);
				}
			}
			Log.i(LOG_TAG,"### Last PID= " + pid);				
				
			//今のPIDを保存する
			pid = android.os.Process.myPid();
			sp = getSharedPreferences("yamruby", MODE_PRIVATE);
			SharedPreferences.Editor editor = sp.edit();
			editor.putInt("PID", pid);
			editor.commit();				
				
			//Maridaを明示的に呼び出します
			Intent marida = new Intent(Intent.ACTION_VIEW);
			marida.setClassName("com.luaridaworks.marida2", "com.luaridaworks.marida2.MainActivity");
			marida.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);

			boolean maridaFlg = true;
			try{
				startActivity(marida);
			}
			catch(Exception e){
				e.printStackTrace();
				maridaFlg = false;
			}
			
			//Maridaが無いときはGooglePlayを呼び出す
			if(maridaFlg==false){
				Toast.makeText(getApplicationContext(), getString(R.string.errMarida), Toast.LENGTH_LONG).show();
				Intent in = new Intent(Intent.ACTION_VIEW, Uri.parse("market://details?id=com.luaridaworks.marida2"));
				try{
					startActivity(in);
				}
				catch(Exception e){
					e.printStackTrace();
					Log.e(LOG_TAG,"market://details ERROR");
				}
				finish();
				return;
			}
			else{
				if((shortcutFount & 1)!=0){
					Log.i(LOG_TAG,"from Intent.ACTION_MAIN ");				
				}
				else if( shortcutFount==2 ){
					Toast.makeText(getApplicationContext(), getString(R.string.errShortcut1), Toast.LENGTH_LONG).show();
					Toast.makeText(getApplicationContext(), getString(R.string.errShortcut2), Toast.LENGTH_LONG).show();
				}				
				
				//mruby VMを起動する
				new Thread(new Runnable(){
					@Override
					public void run()
					{
						Log.i(LOG_TAG,"run() PID= " + android.os.Process.myPid());				
	
						//5sec間実行を試みます
						long twosec = System.currentTimeMillis() + 5000;
	
						while( twosec>System.currentTimeMillis() ){
							//100ms待ちます
							try {
								Thread.sleep(100);
							} catch (InterruptedException e) {
								e.printStackTrace();
							}
	
							//mruby VMの動作開始
							VMStart(SDCardDrive, ScriptFilename);					
						}
						finish();
					}
				}).start();
			}
		}
		else if(Intent.ACTION_CREATE_SHORTCUT.equals(getIntent().getAction())){
			Log.i(LOG_TAG, "Running Shortcut");

			//ショートカット作成から呼ばれた
			RunningShortcut = true;

			Intent intent = new Intent(this, SelectFile.class);

			Bundle bundle = new Bundle();
			bundle.putString("SHORT_CUT", "START");
			intent.putExtras(bundle);

			int requestCode = 1;
			startActivityForResult(intent, requestCode);

			return;		
		}
		//アクティビティを裏に回す
		moveTaskToBack(true);
		Log.i(LOG_TAG, "moveTaskToBack(true)");
	}

	//============================================
	// SelectFileクラが終了したときに呼ばれるコールバック
	//============================================
	@Override
	protected void onActivityResult(int requestCode, int resultCode, Intent data) {
		super.onActivityResult(requestCode, resultCode, data);

		RunningShortcut = false;
		finish();
	}

	//============================================
	// Pause
	//============================================
	@Override
	public void onPause(){
		super.onPause();

		//ショートカットが走っているときは抜ける
		if(RunningShortcut==true){ return; }
	}
	
	//============================================
	// Stop
	//============================================
	@Override
	public void onStop(){
		super.onStop();

		//ショートカットが走っているときは抜ける
		if(RunningShortcut==true){ return; }
	}
	
	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		// Inflate the menu; this adds items to the action bar if it is present.
		getMenuInflater().inflate(R.menu.main, menu);
		return true;
	}

}
