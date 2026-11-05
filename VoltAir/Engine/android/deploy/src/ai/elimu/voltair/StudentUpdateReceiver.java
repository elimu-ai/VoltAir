package ai.elimu.voltair;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.util.Log;
import java.util.ArrayList;

public class StudentUpdateReceiver extends BroadcastReceiver {


    public static final String PREF_STUDENT_LETTERS = "pref_student_letters";
    public static final String PREF_STUDENT_NUMBERS = "pref_student_numbers";

    private VoltAirActivity mActivity = null;

    public void setActivity(VoltAirActivity activity) {
        mActivity = activity;
    }

    @Override
    public void onReceive(Context context, Intent intent) {
        Log.i(getClass().getName(), "onReceive");

        // Customize the user interface to match the current Student's level

        String availableLetters = intent.getExtras().getString("availableLetters");
        Log.i(getClass().getName(), "availableLetters: " + availableLetters);

        if (mActivity != null) {
            mActivity.studentUpdateReceiver(availableLetters);
        }
    }
}
