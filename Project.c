// AI Based Loan Approval System
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
// structure to hold applicant data
typedef struct {
    char name[100];
    int age;
    double monthlyIncome;
    double loanAmount;
    int loanTenureMonths;
    int creditScore;
    double existingEmi;
    int employmentYears;
} Applicant;
// structure to hold evaluation results
typedef struct {
    int score;
    double estimatedEmi;
    double debtToIncomeRatio;
    char decision[20];
    char reasons[500];
} Result;
// global widgets
GtkWidget *entryName;
GtkWidget *entryAge;
GtkWidget *entryIncome;
GtkWidget *entryLoan;
GtkWidget *entryTenure;
GtkWidget *entryCredit;
GtkWidget *entryExistingEmi;
GtkWidget *entryEmployment;

GtkWidget *labelDecision;
GtkWidget *labelScore;
GtkWidget *labelEmi;
GtkWidget *labelDti;
GtkWidget *labelReasons;
GtkWidget *labelRiskLevel;
GtkWidget *decisionBox;
GtkWidget *scoreProgress;
GtkWidget *riskMeter;

double currentDti = 0.0;
// CSS styles for the application
const char *APP_CSS =
    "window { background: #f3f6fb; font-family: Sans; font-size: 14px; }"
    ".header { background: #0b2f5b; color: white; padding: 20px; }"
    ".title { color: white; font-size: 26px; font-weight: bold; }"
    ".subtitle { color: #c9d8ea; font-size: 12px; }"
    ".panel { background: #ffffff; border: 1px solid #d8e0ea; border-radius: 8px; padding: 16px; }"
    ".section-title { color: #0b2f5b; font-size: 17px; font-weight: bold; margin-bottom: 8px; }"
    ".field-label { color: #334155; font-weight: bold; }"
    ".result-box { border-radius: 8px; padding: 16px; }"
    ".decision-default { background: #e8eef6; color: #0f172a; }"
    ".decision-approved { background: #dcfce7; color: #166534; }"
    ".decision-review { background: #ffedd5; color: #9a3412; }"
    ".decision-rejected { background: #fee2e2; color: #991b1b; }"
    ".decision-text { font-size: 26px; font-weight: bold; }"
    ".metric-label { color: #475569; font-weight: bold; }"
    ".factors { background: #f8fafc; color: #1e293b; border: 1px solid #e2e8f0; border-radius: 6px; padding: 12px; }"
    ".primary-button { background: #0b5cab; color: white; border-radius: 6px; padding: 8px 14px; font-weight: bold; }"
    ".secondary-button { background: #e2e8f0; color: #0f172a; border-radius: 6px; padding: 8px 14px; }"
    ".demo-button { background: #0f766e; color: white; border-radius: 6px; padding: 8px 14px; }"
    ".score-low progress { background: #dc2626; }"
    ".score-medium progress { background: #f59e0b; }"
    ".score-high progress { background: #16a34a; }"
    "progressbar trough { min-height: 14px; border-radius: 8px; background: #e5e7eb; }"
    "progressbar progress { min-height: 14px; border-radius: 8px; }";

void addStyle(GtkWidget *widget, const char *className) {
    gtk_style_context_add_class(
        gtk_widget_get_style_context(widget),
        className);
}

void removeStyle(GtkWidget *widget, const char *className) {
    gtk_style_context_remove_class(
        gtk_widget_get_style_context(widget),
        className);
}

void setDecisionStyle(const char *decision) {
    removeStyle(decisionBox, "decision-approved");
    removeStyle(decisionBox, "decision-review");
    removeStyle(decisionBox, "decision-rejected");
    removeStyle(decisionBox, "decision-default");

    if (strcmp(decision, "APPROVED") == 0)
        addStyle(decisionBox, "decision-approved");
    else if (strcmp(decision, "REVIEW") == 0)
        addStyle(decisionBox, "decision-review");
    else if (strcmp(decision, "REJECTED") == 0)
        addStyle(decisionBox, "decision-rejected");
    else
        addStyle(decisionBox, "decision-default");
}

void setProgressStyle(int score) {
    removeStyle(scoreProgress, "score-low");
    removeStyle(scoreProgress, "score-medium");
    removeStyle(scoreProgress, "score-high");

    if (score >= 75)
        addStyle(scoreProgress, "score-high");
    else if (score >= 55)
        addStyle(scoreProgress, "score-medium");
    else
        addStyle(scoreProgress, "score-low");
}

void loadCss(void) {
    GtkCssProvider *provider = gtk_css_provider_new();

    gtk_css_provider_load_from_data(
        provider,
        APP_CSS,
        -1,
        NULL);

    gtk_style_context_add_provider_for_screen(
        gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    g_object_unref(provider);
}

double calculateEmi(double principal, int months) {
    double annualRate = 10.0;
    double monthlyRate = annualRate / (12 * 100.0);
    double power = pow(1 + monthlyRate, months);

    return principal * monthlyRate * power / (power - 1);
}

void showErrorDialog(const char *message) {
    GtkWidget *dialog =
        gtk_message_dialog_new(
            NULL,
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_OK,
            "%s",
            message);

    gtk_window_set_title(GTK_WINDOW(dialog), "Input Error");
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}// function to read applicant data from form
void readApplicantFromForm(Applicant *a) {
    const char *nameText = gtk_entry_get_text(GTK_ENTRY(entryName));

    strncpy(a->name, nameText, sizeof(a->name) - 1);
    a->name[sizeof(a->name) - 1] = '\0';

    a->age = atoi(gtk_entry_get_text(GTK_ENTRY(entryAge)));
    a->monthlyIncome = atof(gtk_entry_get_text(GTK_ENTRY(entryIncome)));
    a->loanAmount = atof(gtk_entry_get_text(GTK_ENTRY(entryLoan)));
    a->loanTenureMonths = atoi(gtk_entry_get_text(GTK_ENTRY(entryTenure)));
    a->creditScore = atoi(gtk_entry_get_text(GTK_ENTRY(entryCredit)));
    a->existingEmi = atof(gtk_entry_get_text(GTK_ENTRY(entryExistingEmi)));
    a->employmentYears = atoi(gtk_entry_get_text(GTK_ENTRY(entryEmployment)));
}
// function to validate applicant data
int validateApplicant(Applicant *a) {
    if (strlen(a->name) == 0) {
        showErrorDialog("Name cannot be empty.");
        return 0;
    }
    if (a->age < 21 || a->age > 60) {
        showErrorDialog("Age must be between 21 and 60.");
        return 0;
    }
    if (a->monthlyIncome <= 0) {
        showErrorDialog("Monthly income must be greater than 0.");
        return 0;
    }
    if (a->loanAmount <= 0) {
        showErrorDialog("Loan amount must be greater than 0.");
        return 0;
    }
    if (a->loanTenureMonths <= 0) {
        showErrorDialog("Loan tenure must be greater than 0.");
        return 0;
    }
    if ((a->creditScore != 0) &&
        (a->creditScore < 300 || a->creditScore > 900)) {
        showErrorDialog("Credit score must be 0 or between 300 and 900.");
        return 0;
    }
    if (a->existingEmi < 0) {
        showErrorDialog("Existing EMI cannot be negative.");
        return 0;
    }
    if (a->employmentYears < 0) {
        showErrorDialog("Employment years cannot be negative.");
        return 0;
    }

    return 1;
}
// function to save loan record to file
void saveLoanRecord(Applicant *a, Result *r) {
    FILE *file = fopen("loan_records.txt", "a");

    if (file == NULL) {
        showErrorDialog("Unable to save loan record.");
        return;
    }

    fprintf(file,
            "Applicant Name: %s\n"
            "Age: %d\n"
            "Monthly Income: %.2f\n"
            "Loan Amount: %.2f\n"
            "Loan Tenure: %d\n"
            "Credit Score: %d\n"
            "Existing EMI: %.2f\n"
            "Employment Years: %d\n"
            "AI Score: %d\n"
            "Final Decision: %s\n"
            "------------------------------\n",
            a->name,
            a->age,
            a->monthlyIncome,
            a->loanAmount,
            a->loanTenureMonths,
            a->creditScore,
            a->existingEmi,
            a->employmentYears,
            r->score,
            r->decision);

    fclose(file);
}

void writeSystemLog(Applicant *a, Result *r) {
    FILE *file = fopen("system_log.txt", "a");
    time_t now;
    struct tm *localTime;
    char timestamp[30];

    if (file == NULL) {
        showErrorDialog("Unable to write system log.");
        return;
    }

    now = time(NULL);
    localTime = localtime(&now);

    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localTime);

    fprintf(file,
            "[%s] %s | %s | Score: %d\n",
            timestamp,
            a->name,
            r->decision,
            r->score);

    fclose(file);
}

// function to evaluate applicant and return result
Result evaluateApplicant(Applicant *a) {
    Result r;

    r.score = 0;
    r.estimatedEmi = calculateEmi(a->loanAmount, a->loanTenureMonths);

    double totalEmi = a->existingEmi + r.estimatedEmi;

    r.debtToIncomeRatio =
        (totalEmi / a->monthlyIncome) * 100;

    strcpy(r.reasons, "");

    if (totalEmi > (a->monthlyIncome * 0.7)) {
        strcpy(r.decision, "REJECTED");
        strcat(r.reasons,
               "Total EMI exceeds 70% of monthly income\n");
        return r;
    }

    if (r.debtToIncomeRatio > 100) {
        strcpy(r.decision, "REJECTED");
        strcat(r.reasons,
               "Debt-To-Income Ratio exceeds 100%\n");
        return r;
    }

    if (a->creditScore == 0) {
        r.score += 15;
        strcat(r.reasons,
               "New Customer - No Credit History\n");
    }
    else if (a->creditScore >= 850) {
        r.score += 35;
        strcat(r.reasons, "Excellent Credit Score (850-900)\n");
    }
    else if (a->creditScore >= 800) {
        r.score += 32;
        strcat(r.reasons, "Very Strong Credit Score (800-849)\n");
    }
    else if (a->creditScore >= 750) {
        r.score += 28;
        strcat(r.reasons, "Strong Credit Score (750-799)\n");
    }
    else if (a->creditScore >= 700) {
        r.score += 24;
        strcat(r.reasons, "Good Credit Score (700-749)\n");
    }
    else if (a->creditScore >= 650) {
        r.score += 20;
        strcat(r.reasons, "Fair Credit Score (650-699)\n");
    }
    else if (a->creditScore >= 600) {
        r.score += 15;
        strcat(r.reasons, "Average Credit Score (600-649)\n");
    }
    else if (a->creditScore >= 550) {
        r.score += 8;
        strcat(r.reasons, "Low Credit Score (550-599)\n");
    }
    else {
        strcpy(r.decision, "REJECTED");
        strcat(r.reasons, "Poor Credit Score\n");
        return r;
    }

    if (r.debtToIncomeRatio <= 35) {
        r.score += 30;
        strcat(r.reasons, "Healthy Debt Ratio\n");
    }
    else if (r.debtToIncomeRatio <= 50) {
        r.score += 18;
        strcat(r.reasons, "Moderate Debt Ratio\n");
    }
    else if (r.debtToIncomeRatio <= 65) {
        r.score += 8;
        strcat(r.reasons, "High Debt Ratio\n");
    }
    else {
        strcat(r.reasons, "Very High Debt Ratio\n");
    }

    if (a->employmentYears >= 5) {
        r.score += 20;
        strcat(r.reasons, "Stable Employment\n");
    }
    else if (a->employmentYears >= 2) {
        r.score += 12;
        strcat(r.reasons, "Moderate Employment Stability\n");
    }
    else if (a->employmentYears >= 1) {
        r.score += 5;
        strcat(r.reasons, "Low Employment Stability\n");
    }
    else {
        strcat(r.reasons, "No Employment Stability\n");
    }

    if (a->age >= 21 && a->age <= 58) {
        r.score += 10;
        strcat(r.reasons, "Eligible Age\n");
    }
    else {
        r.score += 3;
        strcat(r.reasons, "Risky Age Group\n");
    }

    if (a->loanAmount <= a->monthlyIncome * 36) {
        r.score += 5;
        strcat(r.reasons, "Loan Amount Acceptable\n");
    }
    else {
        strcat(r.reasons, "Loan Amount Too High\n");
    }

    if (r.score > 100)
        r.score = 100;

    if (r.score >= 75)
        strcpy(r.decision, "APPROVED");
    else if (r.score >= 55)
        strcpy(r.decision, "REVIEW");
    else
        strcpy(r.decision, "REJECTED");

    return r;
}

GtkWidget *iconLabel(const char *iconName, const char *text, const char *style) {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *icon = gtk_image_new_from_icon_name(iconName, GTK_ICON_SIZE_MENU);
    GtkWidget *label = gtk_label_new(text);

    gtk_label_set_xalign(GTK_LABEL(label), 0.0);
    if (style != NULL)
        addStyle(label, style);

    gtk_box_pack_start(GTK_BOX(box), icon, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), label, TRUE, TRUE, 0);

    return box;
}
// function to create a section with title and content box
GtkWidget *createSection(const char *title) {
    GtkWidget *frame = gtk_frame_new(NULL);
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    GtkWidget *heading = gtk_label_new(title);

    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);
    addStyle(frame, "panel");
    addStyle(heading, "section-title");
    gtk_label_set_xalign(GTK_LABEL(heading), 0.0);

    gtk_container_add(GTK_CONTAINER(frame), box);
    gtk_box_pack_start(GTK_BOX(box), heading, FALSE, FALSE, 0);

    return frame;
}

GtkWidget *sectionBody(GtkWidget *section) {
    return gtk_bin_get_child(GTK_BIN(section));
}

void addInputRow(GtkWidget *grid,
                 int row,
                 const char *iconName,
                 const char *labelText,
                 GtkWidget **entry) {
    GtkWidget *label = iconLabel(iconName, labelText, "field-label");

    *entry = gtk_entry_new();
    gtk_entry_set_width_chars(GTK_ENTRY(*entry), 18);
    gtk_widget_set_hexpand(*entry, TRUE);

    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), *entry, 1, row, 1, 1);
}

GtkWidget *createIconButton(const char *iconName,
                            const char *text,
                            const char *style) {
    GtkWidget *button = gtk_button_new();
    GtkWidget *content = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *icon = gtk_image_new_from_icon_name(iconName, GTK_ICON_SIZE_BUTTON);
    GtkWidget *label = gtk_label_new(text);

    gtk_box_pack_start(GTK_BOX(content), icon, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(content), label, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(button), content);
    addStyle(button, style);

    return button;
}

gboolean drawRiskMeter(GtkWidget *widget, cairo_t *cr, gpointer data) {
    int width = gtk_widget_get_allocated_width(widget);
    int height = gtk_widget_get_allocated_height(widget);
    double cx = width / 2.0;
    double cy = height - 14.0;
    double radius = fmin(width * 0.38, height * 0.78);
    double percent = currentDti / 100.0;
    double angle;
    double nx;
    double ny;

    if (percent < 0.0)
        percent = 0.0;
    if (percent > 1.0)
        percent = 1.0;

    cairo_set_line_width(cr, 14);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);

    cairo_set_source_rgb(cr, 0.86, 0.90, 0.95);
    cairo_arc(cr, cx, cy, radius, M_PI, 2 * M_PI);
    cairo_stroke(cr);

    cairo_set_source_rgb(cr, 0.13, 0.65, 0.32);
    cairo_arc(cr, cx, cy, radius, M_PI, M_PI + (M_PI * 0.35));
    cairo_stroke(cr);

    cairo_set_source_rgb(cr, 0.96, 0.62, 0.04);
    cairo_arc(cr, cx, cy, radius, M_PI + (M_PI * 0.35), M_PI + (M_PI * 0.65));
    cairo_stroke(cr);

    cairo_set_source_rgb(cr, 0.86, 0.15, 0.15);
    cairo_arc(cr, cx, cy, radius, M_PI + (M_PI * 0.65), 2 * M_PI);
    cairo_stroke(cr);

    angle = M_PI + (M_PI * percent);
    nx = cx + cos(angle) * (radius - 6);
    ny = cy + sin(angle) * (radius - 6);

    cairo_set_source_rgb(cr, 0.05, 0.18, 0.36);
    cairo_set_line_width(cr, 4);
    cairo_move_to(cr, cx, cy);
    cairo_line_to(cr, nx, ny);
    cairo_stroke(cr);

    cairo_arc(cr, cx, cy, 5, 0, 2 * M_PI);
    cairo_fill(cr);

    return FALSE;
}
// function to update risk meter text based on DTI value
void updateRiskText(double dti) {
    char text[80];

    if (dti <= 35)
        snprintf(text, sizeof(text), "Risk Meter: Low DTI Risk");
    else if (dti <= 50)
        snprintf(text, sizeof(text), "Risk Meter: Moderate DTI Risk");
    else if (dti <= 65)
        snprintf(text, sizeof(text), "Risk Meter: High DTI Risk");
    else
        snprintf(text, sizeof(text), "Risk Meter: Very High DTI Risk");

    gtk_label_set_text(GTK_LABEL(labelRiskLevel), text);
}
// function to reset result display to default state
void resetResults(void) {
    currentDti = 0.0;
    gtk_label_set_text(GTK_LABEL(labelDecision), "Awaiting Analysis");
    gtk_label_set_text(GTK_LABEL(labelScore), "Loan Eligibility Score: -- / 100");
    gtk_label_set_text(GTK_LABEL(labelEmi), "Estimated EMI: --");
    gtk_label_set_text(GTK_LABEL(labelDti), "Debt To Income Ratio: --");
    gtk_label_set_text(GTK_LABEL(labelReasons), "Key factors will appear after evaluation.");
    gtk_label_set_text(GTK_LABEL(labelRiskLevel), "Risk Meter: --");
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(scoreProgress), 0.0);
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(scoreProgress), "0%");
    setDecisionStyle("");
    setProgressStyle(0);
    gtk_widget_queue_draw(riskMeter);
}

void clearFields(GtkWidget *widget, gpointer data) {
    gtk_entry_set_text(GTK_ENTRY(entryName), "");
    gtk_entry_set_text(GTK_ENTRY(entryAge), "");
    gtk_entry_set_text(GTK_ENTRY(entryIncome), "");
    gtk_entry_set_text(GTK_ENTRY(entryLoan), "");
    gtk_entry_set_text(GTK_ENTRY(entryTenure), "");
    gtk_entry_set_text(GTK_ENTRY(entryCredit), "");
    gtk_entry_set_text(GTK_ENTRY(entryExistingEmi), "");
    gtk_entry_set_text(GTK_ENTRY(entryEmployment), "");
    resetResults();
}


void openCibil(GtkWidget *widget, gpointer data) {
#ifdef _WIN32
    system("start https://www.cibil.com/");
#elif __APPLE__
    system("open https://www.cibil.com/");
#else
    system("xdg-open https://www.cibil.com/");
#endif
}
// function to load demo data into form fields
void loadDemo(GtkWidget *widget, gpointer data) {
    gtk_entry_set_text(GTK_ENTRY(entryName), "Pradeep");
    gtk_entry_set_text(GTK_ENTRY(entryAge), "30");
    gtk_entry_set_text(GTK_ENTRY(entryIncome), "65000");
    gtk_entry_set_text(GTK_ENTRY(entryLoan), "800000");
    gtk_entry_set_text(GTK_ENTRY(entryTenure), "60");
    gtk_entry_set_text(GTK_ENTRY(entryCredit), "735");
    gtk_entry_set_text(GTK_ENTRY(entryExistingEmi), "8000");
    gtk_entry_set_text(GTK_ENTRY(entryEmployment), "6");
}

void checkLoan(GtkWidget *widget, gpointer data) {
    Applicant a;
    Result r;
    char temp[200];

    readApplicantFromForm(&a);

    if (!validateApplicant(&a))
        return;

    r = evaluateApplicant(&a);

    snprintf(temp, sizeof(temp), "%s", r.decision);
    gtk_label_set_text(GTK_LABEL(labelDecision), temp);
    setDecisionStyle(r.decision);

    snprintf(temp, sizeof(temp), "Loan Eligibility Score: %d / 100", r.score);
    gtk_label_set_text(GTK_LABEL(labelScore), temp);

    snprintf(temp, sizeof(temp), "%.0f%%", (double) r.score);
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(scoreProgress),
                                  r.score / 100.0);
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(scoreProgress), temp);
    setProgressStyle(r.score);

    snprintf(temp, sizeof(temp), "Estimated EMI: %.2f", r.estimatedEmi);
    gtk_label_set_text(GTK_LABEL(labelEmi), temp);

    snprintf(temp, sizeof(temp), "Debt To Income Ratio: %.2f%%",
             r.debtToIncomeRatio);
    gtk_label_set_text(GTK_LABEL(labelDti), temp);

    gtk_label_set_text(GTK_LABEL(labelReasons), r.reasons);

    currentDti = r.debtToIncomeRatio;
    updateRiskText(currentDti);
    gtk_widget_queue_draw(riskMeter);

    saveLoanRecord(&a, &r);
    writeSystemLog(&a, &r);
}

GtkWidget *buildHeader(void) {
    GtkWidget *header = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    GtkWidget *title = gtk_label_new("AI Based Loan Approval System");
    GtkWidget *subtitle =
        gtk_label_new("Applicant evaluation, EMI analysis, risk scoring and file logging");

    addStyle(header, "header");
    addStyle(title, "title");
    addStyle(subtitle, "subtitle");
    gtk_label_set_xalign(GTK_LABEL(title), 0.0);
    gtk_label_set_xalign(GTK_LABEL(subtitle), 0.0);

    gtk_box_pack_start(GTK_BOX(header), title, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(header), subtitle, FALSE, FALSE, 0);

    return header;
}

GtkWidget *buildApplicantSection(void) {
    GtkWidget *section = createSection("Applicant Details");
    GtkWidget *grid = gtk_grid_new();

    gtk_grid_set_row_spacing(GTK_GRID(grid), 12);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 16);
    gtk_box_pack_start(GTK_BOX(sectionBody(section)), grid, FALSE, FALSE, 0);

    addInputRow(grid, 0, "avatar-default", "Applicant Name", &entryName);
    addInputRow(grid, 1, "appointment-new", "Age", &entryAge);
    addInputRow(grid, 2, "emblem-photos", "Employment Years", &entryEmployment);

    return section;
}

GtkWidget *buildFinancialSection(void) {
    GtkWidget *section = createSection("Financial Details");
    GtkWidget *grid = gtk_grid_new();

    gtk_grid_set_row_spacing(GTK_GRID(grid), 12);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 16);
    gtk_box_pack_start(GTK_BOX(sectionBody(section)), grid, FALSE, FALSE, 0);

    addInputRow(grid, 0, "go-home", "Monthly Income", &entryIncome);
    addInputRow(grid, 1, "document-open", "Loan Amount", &entryLoan);
    addInputRow(grid, 2, "view-refresh", "Loan Tenure Months", &entryTenure);
    addInputRow(grid, 3, "emblem-favorite", "Credit Score", &entryCredit);
    addInputRow(grid, 4, "edit-undo", "Existing EMI", &entryExistingEmi);

    return section;
}

GtkWidget *buildButtonRow(void) {
    GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    GtkWidget *btnCheck =
        createIconButton("system-search", "Check Loan", "primary-button");
    GtkWidget *btnClear =
        createIconButton("edit-clear", "Clear", "secondary-button");
    GtkWidget *btnDemo =
        createIconButton("document-new", "Load Demo", "demo-button");
    GtkWidget *btnCibil =
    createIconButton("applications-internet",
                     "Check CIBIL",
                     "demo-button");

    gtk_box_pack_start(GTK_BOX(row), btnCheck, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(row), btnClear, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(row), btnDemo, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(row), btnCibil, FALSE, FALSE, 0);
    g_signal_connect(btnCheck, "clicked", G_CALLBACK(checkLoan), NULL);
    g_signal_connect(btnClear, "clicked", G_CALLBACK(clearFields), NULL);
    g_signal_connect(btnDemo, "clicked", G_CALLBACK(loadDemo), NULL);
    g_signal_connect(btnCibil, "clicked", G_CALLBACK(openCibil), NULL);

    return row;
}

GtkWidget *buildResultSection(void) {
    GtkWidget *section = createSection("Loan Assessment Result");
    GtkWidget *body = sectionBody(section);
    GtkWidget *metricGrid = gtk_grid_new();
    GtkWidget *factorsTitle;

    decisionBox = gtk_event_box_new();
    labelDecision = gtk_label_new("Awaiting Analysis");
    addStyle(decisionBox, "result-box");
    addStyle(decisionBox, "decision-default");
    addStyle(labelDecision, "decision-text");
    gtk_container_add(GTK_CONTAINER(decisionBox), labelDecision);

    labelScore = gtk_label_new("Loan Eligibility Score: -- / 100");
    labelEmi = gtk_label_new("Estimated EMI: --");
    labelDti = gtk_label_new("Debt To Income Ratio: --");
    labelRiskLevel = gtk_label_new("Risk Meter: --");
    labelReasons = gtk_label_new("Key factors will appear after evaluation.");

    gtk_label_set_xalign(GTK_LABEL(labelScore), 0.0);
    gtk_label_set_xalign(GTK_LABEL(labelEmi), 0.0);
    gtk_label_set_xalign(GTK_LABEL(labelDti), 0.0);
    gtk_label_set_xalign(GTK_LABEL(labelRiskLevel), 0.0);
    gtk_label_set_xalign(GTK_LABEL(labelReasons), 0.0);
    gtk_label_set_line_wrap(GTK_LABEL(labelReasons), TRUE);

    addStyle(labelScore, "metric-label");
    addStyle(labelEmi, "metric-label");
    addStyle(labelDti, "metric-label");
    addStyle(labelRiskLevel, "metric-label");
    addStyle(labelReasons, "factors");

    scoreProgress = gtk_progress_bar_new();
    gtk_progress_bar_set_show_text(GTK_PROGRESS_BAR(scoreProgress), TRUE);

    riskMeter = gtk_drawing_area_new();
    gtk_widget_set_size_request(riskMeter, 260, 130);
    g_signal_connect(riskMeter, "draw", G_CALLBACK(drawRiskMeter), NULL);

    gtk_grid_set_row_spacing(GTK_GRID(metricGrid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(metricGrid), 12);

    gtk_grid_attach(GTK_GRID(metricGrid),
                    iconLabel("emblem-important", "Decision", "metric-label"),
                    0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(metricGrid), decisionBox, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(metricGrid),
                    iconLabel("view-statistics", "Score", "metric-label"),
                    0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(metricGrid), labelScore, 1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(metricGrid), scoreProgress, 1, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(metricGrid),
                    iconLabel("accessories-calculator", "EMI", "metric-label"),
                    0, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(metricGrid), labelEmi, 1, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(metricGrid),
                    iconLabel("dialog-warning", "DTI", "metric-label"),
                    0, 4, 1, 1);
    gtk_grid_attach(GTK_GRID(metricGrid), labelDti, 1, 4, 1, 1);

    factorsTitle = iconLabel("format-justify-left", "Key Factors", "section-title");

    gtk_box_pack_start(GTK_BOX(body), metricGrid, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(body), riskMeter, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(body), labelRiskLevel, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(body), factorsTitle, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(body), labelReasons, FALSE, FALSE, 0);

    return section;
}

int main(int argc, char *argv[]) {
    GtkWidget *window;
    GtkWidget *mainBox;
    GtkWidget *content;
    GtkWidget *leftColumn;
    GtkWidget *rightColumn;

    gtk_init(&argc, &argv);
    loadCss();

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "AI Based Loan Approval System");
    gtk_window_set_default_size(GTK_WINDOW(window), 980, 760);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);

    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    mainBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    content = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 18);
    leftColumn = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);
    rightColumn = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);

    gtk_container_add(GTK_CONTAINER(window), mainBox);
    gtk_box_pack_start(GTK_BOX(mainBox), buildHeader(), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(mainBox), content, TRUE, TRUE, 18);

    gtk_widget_set_margin_start(content, 18);
    gtk_widget_set_margin_end(content, 18);
    gtk_widget_set_margin_bottom(content, 18);

    gtk_box_pack_start(GTK_BOX(content), leftColumn, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(content), rightColumn, TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(leftColumn), buildApplicantSection(), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(leftColumn), buildFinancialSection(), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(leftColumn), buildButtonRow(), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(rightColumn), buildResultSection(), TRUE, TRUE, 0);

    resetResults();
    gtk_widget_show_all(window);
    gtk_main();

    return 0;
}

