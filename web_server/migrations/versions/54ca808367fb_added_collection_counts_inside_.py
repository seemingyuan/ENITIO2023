"""Added Collection Counts inside Level1Treaure as a fallback option

Revision ID: 54ca808367fb
Revises: 61e66fd649f8
Create Date: 2022-08-01 18:20:39.642863

"""
from alembic import op
import sqlalchemy as sa


# revision identifiers, used by Alembic.
revision = '54ca808367fb'
down_revision = '61e66fd649f8'
branch_labels = None
depends_on = None


def upgrade():
    # ### commands auto generated by Alembic - please adjust! ###
    op.add_column('level1_treasure', sa.Column('num_alatar_collected', sa.Integer(), nullable=True))
    op.add_column('level1_treasure', sa.Column('num_drachen_collected', sa.Integer(), nullable=True))
    op.add_column('level1_treasure', sa.Column('num_eva_collected', sa.Integer(), nullable=True))
    op.add_column('level1_treasure', sa.Column('num_invicta_collected', sa.Integer(), nullable=True))
    # ### end Alembic commands ###


def downgrade():
    # ### commands auto generated by Alembic - please adjust! ###
    op.drop_column('level1_treasure', 'num_invicta_collected')
    op.drop_column('level1_treasure', 'num_eva_collected')
    op.drop_column('level1_treasure', 'num_drachen_collected')
    op.drop_column('level1_treasure', 'num_alatar_collected')
    # ### end Alembic commands ###
